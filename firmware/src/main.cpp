#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include <time.h>
#include "env.h"

// ================== DEBUG OWM (dump payload) ==================
#define OWM_DEBUG_RAW 1          // 0=off, 1=liga dump de payload (primeiros OWM_DUMP_MAX chars)
#define OWM_DUMP_MAX  1200       // quanto mostrar no Serial do corpo

// ========= ANALÓGICOS =========
#define SOIL_PIN 32
#define LDR_PIN  34
const int MV_DRY = 3181;
const int MV_WET = 2300;

// LDR → pH
const int PH_MAX = 14;
int LDR_MV_MIN_DARK   = 200;
int LDR_MV_MAX_BRIGHT = 2800;

// Compensação do ADC
const int ADC_GND_MV = 142;
const int ADC_VCC_MV = 3181;

// ========= BOTÕES (NPK) =========
#define BTN_N 25
#define BTN_P 26
#define BTN_K 27
const unsigned long DEBOUNCE_MS = 80;
int  npkLevelN = 0, npkLevelP = 0, npkLevelK = 0; // 0=baixo,1=ok,2=alto
bool lastRawN   = HIGH, lastRawP   = HIGH, lastRawK   = HIGH;
bool lastStabN  = HIGH, lastStabP  = HIGH, lastStabK  = HIGH;
unsigned long lastChangeN = 0, lastChangeP = 0, lastChangeK = 0;

// ========= RELÉ / BOMBA =========
#define RELAY_PIN 13
const bool RELAY_ACTIVE_LOW = false;

const int  SOIL_ON_PCT  = 40;
const int  SOIL_OFF_PCT = 45;

const unsigned long MIN_ON_MS   = 15UL * 1000;
const unsigned long MIN_OFF_MS  = 10UL * 1000;
const unsigned long ARM_DELAY_MS= 10UL * 1000;
const unsigned long MAX_ON_MS   = 60UL * 1000;

bool pumpOn = false;
unsigned long pumpLastChangeMs = 0;
unsigned long bootMs = 0;

inline void relayWrite(bool on) {
  int level = (RELAY_ACTIVE_LOW ? (on ? LOW : HIGH) : (on ? HIGH : LOW));
  digitalWrite(RELAY_PIN, level);
  pumpOn = on;
  pumpLastChangeMs = millis();
  Serial.printf("[RELE] bomba %s\n", on ? "LIGADA" : "DESLIGADA");
}

// ========= FIREBASE =========
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ========= ESTADO =========
bool wifi_connected = false;
bool ntp_ready = false;
bool fb_ready  = false;

unsigned long last_wifi_try_ms = 0;
unsigned long last_ntp_check_ms = 0;
unsigned long last_csv_ms = 0;
unsigned long last_fs_ms  = 0;

const unsigned long WIFI_RETRY_MS   = 10000;
const unsigned long CSV_PERIOD_MS   = 2000;
const unsigned long FS_PERIOD_MS    = 15000;
const unsigned long NTP_TIMEOUT_MS  = 10000;

// ========= METEO (OpenWeather) =========
unsigned long last_weather_ms = 0;
const unsigned long WEATHER_PERIOD_MS = 15UL * 60UL * 1000; // 15 min
bool needImmediateWeather = false;  // << NOVO: dispara 1ª chamada imediatamente

float  wx_tempC  = NAN;    // temperatura atual (de /weather)
int    wx_code   = -1;     // weather id (2xx/3xx/5xx/6xx indicam precip)
String wx_desc   = "";     // descrição PT-BR
float  wx_pop3h  = NAN;    // POP máx nas próximas 3 janelas (0..1)
float  wx_rain3h = NAN;    // soma de chuva nas próximas 3 janelas (mm)
float  wx_pop_h[3]     = {NAN, NAN, NAN};  // POP % (0..100)
float  wx_precip_h[3]  = {NAN, NAN, NAN};  // mm
String wx_time_h[3]    = {"", "", ""};     // HH:MM

// Cache do último valor válido (evita ficar -1)
int    wx_code_last_ok = -1;
String wx_desc_last_ok = "";

// Estabilidade de rede (aguarda 3s após conectar)
unsigned long wifiStableSinceMs = 0;
static bool netReady() {
  if (WiFi.status() == WL_CONNECTED) {
    if (wifiStableSinceMs == 0) wifiStableSinceMs = millis();
    return (millis() - wifiStableSinceMs) >= 3000; // 3s
  }
  wifiStableSinceMs = 0;
  return false;
}

// ========= POLÍTICA DE CHUVA (RAIN HOLD) =========
#define RAIN_POP_THRESH_PCT   60.0f
#define RAIN_MM_THRESH_3H     0.2f
#define RAIN_HOLD_DURATION_MS (3UL*60UL*60UL*1000UL)

bool rainHold = false;
unsigned long rainHoldUntilMs = 0;
String rainHoldReason = "";

// OpenWeather: id que indica precipitação?
static bool owmIdIsWet(int id) {
  int g = id / 100;
  return (g==2 || g==3 || g==5 || g==6); // trovoada, garoa, chuva, neve
}

// ======== Helpers URL/payload/log ========
static String maskKeyInUrl(String u){
  int i=u.indexOf("appid=");
  if (i>=0){ int j=u.indexOf('&', i); if (j<0) j=u.length();
    u.replace(u.substring(i+6, j), "****");
  }
  return u;
}

// Lê corpo bruto (stream) mesmo quando getString() viria vazio (chunked/TLS).
// Retorna até maxLen chars ou até timeoutMs sem novos bytes.
static String readBodyFromStream(HTTPClient &http, unsigned long timeoutMs=2500, size_t maxLen=OWM_DUMP_MAX){
  Stream* s = http.getStreamPtr();
  String out; out.reserve(maxLen);
  unsigned long t0 = millis();
  while (millis() - t0 < timeoutMs && out.length() < maxLen) {
    while (s->available() && out.length() < maxLen) {
      out += (char)s->read();
      t0 = millis();
    }
    delay(5);
  }
  return out;
}

// Requisita OWM (HTTPS/HTTP), SEMPRE capturando payload (getString + stream).
// Retorna o HTTP code; preenche 'payloadOut'; loga URL mascarada e trecho do payload.
static int owmRequest(const String& pathAndQuery, bool httpsMode, String& payloadOut) {
  payloadOut = "";
  String base = httpsMode ? "https" : "http";
  String url  = base + "://api.openweathermap.org" + pathAndQuery;

  HTTPClient http;
  http.setTimeout(9000);
  http.setUserAgent("ESP32");
  http.addHeader("Accept", "application/json");
  http.addHeader("Accept-Encoding", "identity"); // evita gzip

  bool okBegin = false;
  if (httpsMode) {
    WiFiClientSecure cli; cli.setInsecure();
    okBegin = http.begin(cli, url);
  } else {
    WiFiClient cli;
    okBegin = http.begin(cli, url);
  }

  Serial.printf("[OW] %s %s\n", httpsMode ? "HTTPS" : "HTTP", maskKeyInUrl(url).c_str());
  if (!okBegin) {
    Serial.println("[OW] begin() falhou");
    return -1;
  }

  int code = http.GET();

  // 1ª tentativa: getString normal
  String body = http.getString();

  // Se vazio, tenta ler direto do stream bruto
  if (body.isEmpty()) {
    String raw = readBodyFromStream(http, 3000, OWM_DUMP_MAX);
    if (!raw.isEmpty()) body = raw;
  }

  if (OWM_DEBUG_RAW) {
    String snip = body.substring(0, min((int)body.length(), (int)OWM_DUMP_MAX));
    Serial.printf("[OW] HTTP=%d len=%d payload: %s%s\n",
                  code, body.length(),
                  snip.c_str(),
                  (body.length() > OWM_DUMP_MAX ? " …<trunc>" : ""));
  }

  http.end();
  payloadOut = body;
  return code;
}

// ======== OpenWeather — CURRENT + FORECAST (com retries, payload e cache) ========
static bool fetchOWMCurrent() {
  if (!netReady()) { Serial.println("[OW] /weather: rede ainda não estável"); return false; }
  if (String(OW_API_KEY).length() < 5) { Serial.println("[OW] /weather: OW_API_KEY vazio/curto"); return false; }

  String q = String("/data/2.5/weather?lat=") + OW_LAT +
             "&lon=" + OW_LON +
             "&units=" + OW_UNITS +
             "&lang="  + OW_LANG +
             "&appid=" + OW_API_KEY;

  bool success = false;
  String payload; int httpCode = -1;

  // 2 tentativas HTTPS
  for (int a=1; a<=2 && !success; a++){
    httpCode = owmRequest(q, /*httpsMode=*/true, payload);
    if (httpCode != 200 || payload.isEmpty()) {
      Serial.printf("[OW] /weather HTTPS tentativa %d falhou (HTTP=%d)\n", a, httpCode);
      continue;
    }

    DynamicJsonDocument doc(12288);
    DeserializationError err = deserializeJson(doc, payload);
    if (err) { Serial.printf("[OW] /weather JSON err: %s (tent %d)\n", err.c_str(), a); continue; }

    JsonArray wArr = doc["weather"].as<JsonArray>();
    if (wArr.isNull() || wArr.size()==0 || wArr[0].isNull()) {
      Serial.printf("[OW] /weather sem weather[0] (tent %d)\n", a);
      continue;
    }

    wx_tempC = doc["main"]["temp"] | NAN;
    int id = wArr[0]["id"] | -1;
    String desc = String((const char*)(wArr[0]["description"] | ""));

    if (id != -1) {
      wx_code = id; wx_desc = desc;
      wx_code_last_ok = id; wx_desc_last_ok = desc;
      success = true;
    } else {
      Serial.printf("[OW] /weather sem 'id' (tent %d)\n", a);
    }
  }

  // Fallback 1x HTTP
  if (!success) {
    httpCode = owmRequest(q, /*httpsMode=*/false, payload);
    if (httpCode == 200 && !payload.isEmpty()) {
      DynamicJsonDocument doc(12288);
      if (!deserializeJson(doc, payload)) {
        JsonArray wArr = doc["weather"].as<JsonArray>();
        if (!wArr.isNull() && wArr.size()>0 && !wArr[0].isNull()) {
          int id = wArr[0]["id"] | -1;
          wx_desc = String((const char*)(wArr[0]["description"] | ""));
          wx_tempC = doc["main"]["temp"] | NAN;
          if (id != -1) { wx_code = id; wx_code_last_ok = id; wx_desc_last_ok = wx_desc; success = true; }
        }
      }
    }
  }

  // Fallback cache
  if (!success && wx_code_last_ok != -1) {
    wx_code = wx_code_last_ok; wx_desc = wx_desc_last_ok;
    Serial.println("[OW] Fallback(cache): usando último id/desc OK.");
    success = true;
  }

  if (!success) {
    Serial.println("[OW][FAIL] /weather: wx_code ainda -1 após todas as tentativas.");
  }
  return success;
}

static bool fetchOWMForecast3(int *fallback_id = nullptr, String *fallback_desc = nullptr) {
  if (!netReady()) { Serial.println("[OW] /forecast: rede ainda não estável"); return false; }
  if (String(OW_API_KEY).length() < 5) { Serial.println("[OW] /forecast: OW_API_KEY vazio/curto"); return false; }

  String q = String("/data/2.5/forecast?lat=") + OW_LAT +
             "&lon=" + OW_LON +
             "&cnt=3&units=" + OW_UNITS +
             "&lang="  + OW_LANG +
             "&appid=" + OW_API_KEY;

  bool success = false;
  String payload; int httpCode = -1;

  // 2x HTTPS
  for (int a=1; a<=2 && !success; a++){
    httpCode = owmRequest(q, /*httpsMode=*/true, payload);
    if (httpCode != 200 || payload.isEmpty()) {
      Serial.printf("[OW] /forecast HTTPS tentativa %d falhou (HTTP=%d)\n", a, httpCode);
      continue;
    }

    DynamicJsonDocument doc(20000);
    DeserializationError err = deserializeJson(doc, payload);
    if (err) { Serial.printf("[OW] /forecast JSON err: %s (tent %d)\n", err.c_str(), a); continue; }

    JsonArray list = doc["list"].as<JsonArray>();
    if (list.isNull() || list.size()==0) {
      Serial.printf("[OW] /forecast list vazio (tent %d)\n", a);
      continue;
    }

    for (int k=0;k<3;k++){ wx_pop_h[k]=NAN; wx_precip_h[k]=NAN; wx_time_h[k]=""; }
    float maxPopPct = 0.0f, sumRain = 0.0f;

    for (int i=0; i<list.size() && i<3; i++) {
      JsonObject it = list[i];
      const char* dt_txt = it["dt_txt"] | nullptr;
      if (dt_txt && strlen(dt_txt)>=16) wx_time_h[i] = String(dt_txt).substring(11,16);
      else wx_time_h[i] = (dt_txt? String(dt_txt): String(""));

      float pop = it["pop"] | 0.0f; wx_pop_h[i] = pop * 100.0f;
      if (wx_pop_h[i] > maxPopPct) maxPopPct = wx_pop_h[i];

      float r3h = it["rain"]["3h"] | 0.0f; wx_precip_h[i] = r3h; sumRain += r3h;

      if (i==0 && fallback_id && fallback_desc) {
        JsonArray wArr = it["weather"].as<JsonArray>();
        if (!wArr.isNull() && wArr.size()>0 && !wArr[0].isNull()) {
          *fallback_id   = wArr[0]["id"] | -1;
          *fallback_desc = String((const char*)(wArr[0]["description"] | ""));
        }
      }
    }

    wx_pop3h  = maxPopPct / 100.0f;
    wx_rain3h = sumRain;
    success = true;
  }

  // 1x HTTP fallback
  if (!success) {
    httpCode = owmRequest(q, /*httpsMode=*/false, payload);
    if (httpCode == 200 && !payload.isEmpty()) {
      DynamicJsonDocument doc(20000);
      if (!deserializeJson(doc, payload)) {
        JsonArray list = doc["list"].as<JsonArray>();
        if (!list.isNull() && list.size()>0) {
          for (int k=0;k<3;k++){ wx_pop_h[k]=NAN; wx_precip_h[k]=NAN; wx_time_h[k]=""; }
          float maxPopPct = 0.0f, sumRain = 0.0f;
          for (int i=0; i<list.size() && i<3; i++) {
            JsonObject it = list[i];
            const char* dt_txt = it["dt_txt"] | nullptr;
            if (dt_txt && strlen(dt_txt)>=16) wx_time_h[i] = String(dt_txt).substring(11,16);
            else wx_time_h[i] = (dt_txt? String(dt_txt): String(""));
            float pop = it["pop"] | 0.0f; wx_pop_h[i] = pop*100.0f;
            if (wx_pop_h[i] > maxPopPct) maxPopPct = wx_pop_h[i];
            float r3h = it["rain"]["3h"] | 0.0f; wx_precip_h[i] = r3h; sumRain += r3h;

            if (i==0 && fallback_id && fallback_desc) {
              JsonArray wArr = it["weather"].as<JsonArray>();
              if (!wArr.isNull() && wArr.size()>0 && !wArr[0].isNull()) {
                *fallback_id   = wArr[0]["id"] | -1;
                *fallback_desc = String((const char*)(wArr[0]["description"] | ""));
              }
            }
          }
          wx_pop3h  = maxPopPct / 100.0f;
          wx_rain3h = sumRain;
          success = true;
        }
      }
    }
  }

  if (!success) {
    Serial.println("[OW][FAIL] /forecast não trouxe lista válida.");
  }
  return success;
}

bool fetchWeather() {
  Serial.println("[OW] Iniciando fetchWeather()");
  int fb_id = -1; String fb_desc = "";
  bool okFct = fetchOWMForecast3(&fb_id, &fb_desc); // POP/chuva + possível fallback
  bool okCur = fetchOWMCurrent();                   // id/desc/temp

  // Fallback: se /weather falhou ou não trouxe id, usa o do forecast
  if ((wx_code == -1) && (fb_id != -1)) {
    wx_code = fb_id;
    wx_desc = fb_desc;
    wx_code_last_ok = wx_code;
    wx_desc_last_ok = wx_desc;
    Serial.printf("[OW] Fallback: usando id=%d (%s) do /forecast\n", wx_code, wx_desc.c_str());
  }

  // Se ainda -1, tenta último ok
  if (wx_code == -1 && wx_code_last_ok != -1) {
    wx_code = wx_code_last_ok;
    wx_desc = wx_desc_last_ok;
    Serial.printf("[OW] Fallback: usando último OK id=%d (%s)\n", wx_code, wx_desc.c_str());
  }

  // Logs compactos
  String popStr = isnan(wx_pop3h) ? String("NA") : String(wx_pop3h*100.0f, 0);
  Serial.printf("[OW] T=%.1fC id=%d (%s) | POPmax3h=%s%% | RAIN3h=%.1fmm\n",
                wx_tempC, wx_code, wx_desc.c_str(), popStr.c_str(),
                (isnan(wx_rain3h)?0.0f:wx_rain3h));
  Serial.printf("[OW.h] %s: POP=%.0f%% PR=%.1fmm | %s: POP=%.0f%% PR=%.1fmm | %s: POP=%.0f%% PR=%.1fmm\n",
                wx_time_h[0].c_str(), (isnan(wx_pop_h[0])?0.0f:wx_pop_h[0]), (isnan(wx_precip_h[0])?0.0f:wx_precip_h[0]),
                wx_time_h[1].c_str(), (isnan(wx_pop_h[1])?0.0f:wx_pop_h[1]), (isnan(wx_precip_h[1])?0.0f:wx_precip_h[1]),
                wx_time_h[2].c_str(), (isnan(wx_pop_h[2])?0.0f:wx_pop_h[2]), (isnan(wx_precip_h[2])?0.0f:wx_precip_h[2]));

  // Arma RainHold se necessário
  const float maxPopPct = isnan(wx_pop3h)? 0.0f : wx_pop3h*100.0f;
  const float sumRain   = isnan(wx_rain3h)? 0.0f : wx_rain3h;
  bool wetByCode = (wx_code!=-1) && owmIdIsWet(wx_code);
  bool wetByPop  = maxPopPct >= RAIN_POP_THRESH_PCT;
  bool wetByMm   = sumRain   >= RAIN_MM_THRESH_3H;

  if (wetByCode || wetByPop || wetByMm) {
    rainHold = true;
    rainHoldUntilMs = millis() + RAIN_HOLD_DURATION_MS;
    if (wetByCode)      rainHoldReason = String("owm_id_") + wx_code;
    else if (wetByPop)  rainHoldReason = String("pop>=") + String(RAIN_POP_THRESH_PCT,0) + "%";
    else                rainHoldReason = String("rain3h>=") + String(RAIN_MM_THRESH_3H,1) + "mm";
  }

  if (wx_code == -1) {
    Serial.println("[OW][FAIL] wx_code permanece -1 após current/forecast/cache. Veja payloads acima.");
  }

  return okCur || okFct;
}

// ========= UTILS =========
static inline float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  if (in_max == in_min) return out_min;
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
int medianMvPin(int pin, int n = 21) {
  int v[41]; if (n > 41) n = 41;
  for (int i = 0; i < n; i++) { v[i] = analogReadMilliVolts(pin); delay(2); }
  for (int i = 1; i < n; i++) { int x=v[i], j=i-1; while (j>=0 && v[j]>x){ v[j+1]=v[j]; j--; } v[j+1]=x; }
  return v[n/2];
}
int readLDRmVComp() {
  int mv_raw = medianMvPin(LDR_PIN);
  float mv = (mv_raw - ADC_GND_MV) * (3300.0f / (ADC_VCC_MV - ADC_GND_MV));
  if (mv < 0) mv = 0;
  if (mv > 3300) mv = 3300;
  return (int)mv;
}
String iso8601NowUTC() {
  time_t now; time(&now);
  struct tm tm_utc; gmtime_r(&now, &tm_utc);
  char buf[30]; strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
  return String(buf);
}

// ========= WIFI / NTP / FB =========
void tryConnectWiFi() {
  if (millis() - last_wifi_try_ms < WIFI_RETRY_MS) return;
  last_wifi_try_ms = millis();
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifi_connected) {
      wifi_connected = true;
      wifiStableSinceMs = millis(); // começa a contar estabilidade
      Serial.printf("WiFi OK, IP: %s, RSSI=%d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
      Serial.printf("[CFG] lat=%s lon=%s units=%s lang=%s\n", OW_LAT, OW_LON, OW_UNITS, OW_LANG);
      needImmediateWeather = true; // << NOVO: dispara 1ª consulta meteo já
    }
    return;
  }
  if (!wifi_connected) Serial.printf("Conectando WiFi SSID='%s'...\n", WIFI_SSID);
  else Serial.println("WiFi caiu, tentando reconectar...");
  WiFi.mode(WIFI_STA); WiFi.persistent(false); WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}
void trySetupTime() {
  if (ntp_ready || !wifi_connected) return;
  if (last_ntp_check_ms == 0) { Serial.println("Sincronizando NTP..."); configTime(0, 0, "pool.ntp.org", "time.nist.gov"); last_ntp_check_ms = millis(); return; }
  time_t now; time(&now); struct tm tm_info; gmtime_r(&now, &tm_info);
  if (tm_info.tm_year > (2016 - 1900)) { ntp_ready = true; Serial.println("NTP OK"); }
  else if (millis() - last_ntp_check_ms > NTP_TIMEOUT_MS) { Serial.println("[WARN] NTP timeout, seguirá tentando depois."); last_ntp_check_ms = 0; }
}
void tryInitFirebase() {
  if (fb_ready || !wifi_connected) return;
  Serial.println("Autenticando Firebase...");
  config.api_key = API_KEY; auth.user.email = USER_EMAIL; auth.user.password = USER_PASSWORD;
  Firebase.begin(&config, &auth); Firebase.reconnectNetwork(true);
  unsigned long t0 = millis();
  while (auth.token.uid == "") { if (millis() - t0 > 15000) { Serial.println("[ERRO] Timeout Firebase."); return; } delay(300); Serial.print("."); }
  Serial.printf("\nFirebase OK. UID: %s\n", auth.token.uid.c_str()); fb_ready = true;
}

// ========= FIRESTORE (leituras) =========
bool writeToFirestore(int soil_mv, int soil_pct, int ldr_mv, float ph,
                      int n_lvl, int p_lvl, int k_lvl, bool pump_on,
                      const String& isoTs) {
  FirebaseJson content, fields;
  fields.set("soil_mv/integerValue", soil_mv);
  fields.set("soil_pct/integerValue", soil_pct);
  fields.set("ldr_mv/integerValue", ldr_mv);
  fields.set("ph/doubleValue", ph);
  fields.set("npk_n/integerValue", n_lvl);
  fields.set("npk_p/integerValue", p_lvl);
  fields.set("npk_k/integerValue", k_lvl);
  fields.set("pump_on/booleanValue", pump_on);

  // flags de chuva
  fields.set("rain_hold/booleanValue", rainHold);
  if (rainHold) {
    unsigned long ms_left = (rainHoldUntilMs > millis()) ? (rainHoldUntilMs - millis()) : 0;
    time_t now; time(&now);
    time_t until = now + (ms_left/1000);
    struct tm tm_utc; gmtime_r(&until, &tm_utc);
    char buf[30]; strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
    fields.set("rain_hold_until/stringValue", String(buf));
  }

  fields.set("ts/timestampValue", isoTs);
  fields.set("dev/stringValue", "esp32_01");
  content.set("fields", fields);

  String mask = "soil_mv,soil_pct,ldr_mv,ph,npk_n,npk_p,npk_k,pump_on,rain_hold,rain_hold_until,ts,dev";
  bool ok = Firebase.Firestore.patchDocument(&fbdo, PROJECT_ID, FIRESTORE_DB,
                                             "leituras/esp32_01", content.raw(), mask.c_str());
  if (!ok) {
    Serial.printf("[Firestore] patch erro: %s\n", fbdo.errorReason().c_str());
    ok = Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, FIRESTORE_DB,
                                           "leituras/esp32_01", content.raw());
    if (!ok) Serial.printf("[Firestore] create erro: %s\n", fbdo.errorReason().c_str());
  }
  bool ok_hist = Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, FIRESTORE_DB,
                                                   "leituras/esp32_01/historico", content.raw());
  if (!ok_hist) Serial.printf("[Firestore] historico erro: %s\n", fbdo.errorReason().c_str());
  return ok && ok_hist;
}

// ========= BOTÕES (edge + debounce) =========
void handleButtonEdge(int pin, bool &lastRaw, bool &lastStable, unsigned long &lastChange, int &level, const char label) {
  bool raw = digitalRead(pin);
  unsigned long now = millis();
  if (raw != lastRaw) { lastChange = now; lastRaw = raw; }
  if ((now - lastChange) >= DEBOUNCE_MS && raw != lastStable) {
    if (lastStable == HIGH && raw == LOW) { level = (level + 1) % 3; Serial.printf("[%c] nível = %d\n", label, level); }
    lastStable = raw;
  }
}

// ========= SETUP =========
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);        // << ajuda a ver logs do WiFiClient
  delay(300);
  Serial.println("\n🌱 Start: solo + NPK(botões) + pH(LDR) + bomba(relé) + OpenWeather + RainHold");

  analogSetPinAttenuation(SOIL_PIN, ADC_11db);
  analogSetPinAttenuation(LDR_PIN,  ADC_11db);

  pinMode(BTN_N, INPUT_PULLUP);
  pinMode(BTN_P, INPUT_PULLUP);
  pinMode(BTN_K, INPUT_PULLUP);

  pinMode(RELAY_PIN, OUTPUT);
  relayWrite(false);
  bootMs = millis();

  last_wifi_try_ms = 0;
  tryConnectWiFi();
}

// ========= LOOP =========
void loop() {
  // 0) Botões
  handleButtonEdge(BTN_N, lastRawN, lastStabN, lastChangeN, npkLevelN, 'N');
  handleButtonEdge(BTN_P, lastRawP, lastStabP, lastChangeP, npkLevelP, 'P');
  handleButtonEdge(BTN_K, lastRawK, lastStabK, lastChangeK, npkLevelK, 'K');

  // 1) Meteo periódico + 1ª vez imediata (quando rede estável)
  if (wifi_connected && netReady()) {
    if (needImmediateWeather || (millis() - last_weather_ms >= WEATHER_PERIOD_MS)) {
      last_weather_ms = millis();
      needImmediateWeather = false;
      fetchWeather();
    }
  } else if (wifi_connected && !netReady()) {
    Serial.println("[NET] Aguardando rede estabilizar para buscar meteo...");
  }

  // expira hold
  if (rainHold && millis() > rainHoldUntilMs) {
    rainHold = false; rainHoldReason = "";
    Serial.println("[RAIN] Hold expirou; irrigação liberada.");
  }

  // 2) Leituras periódicas + controle bomba
  if (millis() - last_csv_ms >= CSV_PERIOD_MS) {
    last_csv_ms = millis();

    int soil_mv  = medianMvPin(SOIL_PIN);
    int soil_pct = constrain(map(soil_mv, MV_DRY, MV_WET, 0, 100), 0, 100);

    unsigned long now = millis();
    bool armed     = (now - bootMs) >= ARM_DELAY_MS;
    bool canOn     = (!pumpOn) && (now - pumpLastChangeMs >= MIN_OFF_MS);
    bool canOff    = ( pumpOn) && (now - pumpLastChangeMs >= MIN_ON_MS);
    bool maxOnHit  = ( pumpOn) && (now - pumpLastChangeMs >= MAX_ON_MS);

    if (maxOnHit && canOff) {
      Serial.println("[SAFE] Tempo máximo ligado atingido. Desligando.");
      relayWrite(false);
    } else if (armed) {
      if (rainHold) {
        if (pumpOn && canOff) {
          Serial.printf("[RULE] RainHold ativo (%s) → desligando bomba.\n", rainHoldReason.c_str());
          relayWrite(false);
        }
      } else {
        if (!pumpOn && soil_pct < SOIL_ON_PCT && canOn)          relayWrite(true);
        else if (pumpOn && soil_pct >= SOIL_OFF_PCT && canOff)   relayWrite(false);
      }
    }

    int   ldr_mv = readLDRmVComp();
    float ph     = fmap((float)ldr_mv, (float)LDR_MV_MIN_DARK, (float)LDR_MV_MAX_BRIGHT, 0.0f, (float)PH_MAX);
    ph = constrain(ph, 0.0f, (float)PH_MAX);

    String ts = ntp_ready ? iso8601NowUTC() : String("1970-01-01T00:00:00Z");

    // LOG humano
    Serial.printf("SOLO mV=%4d Umid=%3d%% | LDR mV=%4d pH=%.2f | N=%d P=%d K=%d | PUMP=%d | HOLD=%d | %s\n",
                  soil_mv, soil_pct, ldr_mv, ph, npkLevelN, npkLevelP, npkLevelK,
                  pumpOn ? 1 : 0, rainHold ? 1 : 0, ts.c_str());

    // CSVX (para scripts)
    Serial.printf("CSVX,%s,%d,%d,%d,%.2f,%d,%d,%d,%d,%d,%d\n",
                  ts.c_str(), soil_mv, soil_pct, ldr_mv, ph,
                  npkLevelN, npkLevelP, npkLevelK,
                  pumpOn ? 1 : 0, rainHold ? 1 : 0, wx_code);

    // LOG meteo resumido
    String popStr = isnan(wx_pop3h) ? String("NA") : String(wx_pop3h*100.0f, 0);
    Serial.printf("OW{T=%.1fC POPmax=%s%% RAIN3h=%.1fmm id=%d %s}\n",
                  wx_tempC, popStr.c_str(), (isnan(wx_rain3h)?0.0f:wx_rain3h), wx_code, wx_desc.c_str());
    Serial.printf("OW.h [%s] POP=%.0f%% PR=%.1fmm | [%s] POP=%.0f%% PR=%.1fmm | [%s] POP=%.0f%% PR=%.1fmm\n",
                  wx_time_h[0].c_str(), (isnan(wx_pop_h[0])?0.0f:wx_pop_h[0]), (isnan(wx_precip_h[0])?0.0f:wx_precip_h[0]),
                  wx_time_h[1].c_str(), (isnan(wx_pop_h[1])?0.0f:wx_pop_h[1]), (isnan(wx_precip_h[1])?0.0f:wx_precip_h[1]),
                  wx_time_h[2].c_str(), (isnan(wx_pop_h[2])?0.0f:wx_pop_h[2]), (isnan(wx_precip_h[2])?0.0f:wx_precip_h[2]));

    // Firestore leituras
    if (wifi_connected && fb_ready && (millis() - last_fs_ms >= FS_PERIOD_MS)) {
      last_fs_ms = millis();
      if (writeToFirestore(soil_mv, soil_pct, ldr_mv, ph, npkLevelN, npkLevelP, npkLevelK, pumpOn, ts))
        Serial.println("[Firestore] OK");
      else
        Serial.println("[Firestore] Falhou");
    }
  }

  // 3) Wi-Fi / NTP / FB (estado e estabilidade)
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifi_connected) {
      wifi_connected = true;
      wifiStableSinceMs = millis();
      Serial.printf("[WiFi] Conectado. IP=%s RSSI=%d dBm\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      Serial.printf("[CFG] lat=%s lon=%s units=%s lang=%s\n", OW_LAT, OW_LON, OW_UNITS, OW_LANG);
      needImmediateWeather = true; // garante 1ª consulta após conectar
    }
  } else {
    if (wifi_connected) Serial.println("[WiFi] Desconectado.");
    wifi_connected = false;
    wifiStableSinceMs = 0;
    tryConnectWiFi();
  }
  trySetupTime();
  tryInitFirebase();

  delay(5);
}
