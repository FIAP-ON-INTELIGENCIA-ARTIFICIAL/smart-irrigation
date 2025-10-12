#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include <time.h>
#include "env.h"

// ========= ANALÓGICOS =========
#define SOIL_PIN 32
#define LDR_PIN  34
const int MV_DRY = 3181;
const int MV_WET = 2300;

// LDR → pH (ajuste estes limites ao seu divisor)
const int PH_MAX = 14;
int LDR_MV_MIN_DARK   = 200;
int LDR_MV_MAX_BRIGHT = 2800;

// Compensação do ADC (GPIO34 medidos por você)
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

const int  SOIL_ON_PCT  = 40;        // liga < 40%
const int  SOIL_OFF_PCT = 45;        // desliga ≥ 45%

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

// ========= METEO (Open-Meteo) =========
unsigned long last_weather_ms = 0;
const unsigned long WEATHER_PERIOD_MS = 15UL * 60UL * 1000; // 15 min

float  wx_tempC  = NAN;    // temperatura atual
int    wx_code   = -1;     // WMO weather code
String wx_desc   = "";     // descrição em PT
float  wx_pop3h  = NAN;    // prob. precip máx próximas ~3h (0..1)
float  wx_rain3h = NAN;    // precip. soma próximas ~3h (mm)

// NOVO: próximos 3 pontos horários
float  wx_pop_h[3]     = {NAN, NAN, NAN};  // em % (0..100)
float  wx_precip_h[3]  = {NAN, NAN, NAN};  // em mm
String wx_time_h[3]    = {"", "", ""};     // ISO; mostramos HH:MM

// descrição simples dos códigos (resumo)
static const char* wmoDesc(int code){
  switch(code){
    case 0:  return "céu limpo";
    case 1:  return "principalmente claro";
    case 2:  return "parcialmente nublado";
    case 3:  return "nublado";
    case 45: case 48: return "neblina";
    case 51: case 53: case 55: return "garoa";
    case 56: case 57: return "garoa congelante";
    case 61: case 63: case 65: return "chuva";
    case 66: case 67: return "chuva congelante";
    case 71: case 73: case 75: return "neve";
    case 77: return "grãos de neve";
    case 80: case 81: case 82: return "aguaceiros";
    case 85: case 86: return "nevasca";
    case 95: return "trovoadas";
    case 96: case 99: return "trovoadas com granizo";
    default: return "condição desconhecida";
  }
}

// pega dados: current_weather=true + hourly POP/precip
bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();           // simplifica TLS no ESP32
  HTTPClient https;
  https.setTimeout(7000);         // 7s timeout

  String url = String("https://api.open-meteo.com/v1/forecast?")
    + "latitude="   + OM_LAT
    + "&longitude=" + OM_LON
    + "&current_weather=true"
    + "&hourly=precipitation_probability,precipitation"
    + "&forecast_days=1"
    + "&timezone=auto";

  if (!https.begin(client, url)) { Serial.println("[OM] begin() falhou"); return false; }
  int code = https.GET();
  if (code != 200) { Serial.printf("[OM] HTTP %d\n", code); https.end(); return false; }

  // parse (stream)
  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, https.getStream());
  https.end();
  if (err) { Serial.printf("[OM] JSON err: %s\n", err.c_str()); return false; }

  // current_weather
  JsonObject cw = doc["current_weather"];
  if (cw.isNull()) { Serial.println("[OM] current_weather ausente"); return false; }

  wx_code  = cw["weathercode"] | -1;
  wx_tempC = cw["temperature"] | NAN;
  wx_desc  = String(wmoDesc(wx_code));

  // hourly arrays
  JsonArray timeArr = doc["hourly"]["time"].as<JsonArray>();
  JsonArray popArr  = doc["hourly"]["precipitation_probability"].as<JsonArray>();
  JsonArray precArr = doc["hourly"]["precipitation"].as<JsonArray>();

  // zera vetores
  for (int i=0;i<3;i++){ wx_pop_h[i]=NAN; wx_precip_h[i]=NAN; wx_time_h[i]=""; }

  float maxPopPct = 0.0f, sumNext3h = 0.0f;
  const int take = 3;

  for (int i=0;i<take;i++){
    // tempo
    if (!timeArr.isNull() && i < (int)timeArr.size()){
      String t = String((const char*)timeArr[i]);
      // tenta extrair HH:MM de "YYYY-MM-DDTHH:MM"
      if (t.length() >= 16) wx_time_h[i] = t.substring(11,16);
      else wx_time_h[i] = t;
    }

    // POP %
    if (!popArr.isNull() && i < (int)popArr.size()){
      float p = popArr[i] | 0.0f;
      wx_pop_h[i] = p;
      if (p > maxPopPct) maxPopPct = p;
    }

    // precip mm
    if (!precArr.isNull() && i < (int)precArr.size()){
      float mm = precArr[i] | 0.0f;
      wx_precip_h[i] = mm;
      sumNext3h += mm;
    }
  }

  wx_pop3h  = maxPopPct / 100.0f; // 0..1
  wx_rain3h = sumNext3h;          // mm (≈ soma próximas 3h)

  Serial.printf("[OM] T=%.1fC code=%d (%s) | POP3h(max)=%.0f%% | PR3h=%.1fmm\n",
                wx_tempC, wx_code, wx_desc.c_str(), maxPopPct, wx_rain3h);

  // Log detalhado dos 3 próximos horários
  Serial.printf("[OM.h] %s: POP=%.0f%% PR=%.1fmm | %s: POP=%.0f%% PR=%.1fmm | %s: POP=%.0f%% PR=%.1fmm\n",
                wx_time_h[0].c_str(), (isnan(wx_pop_h[0])?0.0f:wx_pop_h[0]), (isnan(wx_precip_h[0])?0.0f:wx_precip_h[0]),
                wx_time_h[1].c_str(), (isnan(wx_pop_h[1])?0.0f:wx_pop_h[1]), (isnan(wx_precip_h[1])?0.0f:wx_precip_h[1]),
                wx_time_h[2].c_str(), (isnan(wx_pop_h[2])?0.0f:wx_pop_h[2]), (isnan(wx_precip_h[2])?0.0f:wx_precip_h[2]));
  return true;
}

// grava meteo em doc separado com caminho VÁLIDO
bool writeWeatherToFirestore(const String& isoTs){
  if (!fb_ready) return false;
  FirebaseJson content, fields;
  if (!isnan(wx_tempC))  fields.set("wx_temp_c/doubleValue", wx_tempC);
  fields.set("wx_code/integerValue", wx_code);
  if (wx_desc.length())  fields.set("wx_desc/stringValue", wx_desc);
  if (!isnan(wx_pop3h))  fields.set("wx_pop3h/doubleValue", wx_pop3h);
  if (!isnan(wx_rain3h)) fields.set("wx_rain3h_mm/doubleValue", wx_rain3h);

  // também manda os 3 próximos pontos (se existirem)
  for (int i=0;i<3;i++){
    if (!isnan(wx_pop_h[i]))    fields.set(("h"+String(i)+"_pop_pct/doubleValue").c_str(), wx_pop_h[i]);
    if (!isnan(wx_precip_h[i])) fields.set(("h"+String(i)+"_precip_mm/doubleValue").c_str(), wx_precip_h[i]);
    if (wx_time_h[i].length())  fields.set(("h"+String(i)+"_time/stringValue").c_str(), wx_time_h[i]);
  }

  fields.set("ts/timestampValue", isoTs);
  fields.set("dev/stringValue", "esp32_01");
  content.set("fields", fields);

  const char* docCurrent = "leituras/esp32_01/weather/current";
  const char* collHist   = "leituras/esp32_01/weather_hist";

  bool ok = Firebase.Firestore.patchDocument(
      &fbdo, PROJECT_ID, FIRESTORE_DB, docCurrent,
      content.raw(),
      "wx_temp_c,wx_code,wx_desc,wx_pop3h,wx_rain3h_mm,h0_pop_pct,h0_precip_mm,h0_time,h1_pop_pct,h1_precip_mm,h1_time,h2_pop_pct,h2_precip_mm,h2_time,ts,dev");
  if (!ok) {
    Serial.printf("[Firestore] weather patch err: %s\n", fbdo.errorReason().c_str());
    ok = Firebase.Firestore.createDocument(
      &fbdo, PROJECT_ID, FIRESTORE_DB, docCurrent, content.raw());
    if (!ok) Serial.printf("[Firestore] weather create err: %s\n", fbdo.errorReason().c_str());
  }

  bool ok_hist = Firebase.Firestore.createDocument(
      &fbdo, PROJECT_ID, FIRESTORE_DB, collHist, content.raw());
  if (!ok_hist) Serial.printf("[Firestore] weather_hist err: %s\n", fbdo.errorReason().c_str());

  return ok && ok_hist;
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

// LDR compensado por 2 pontos (GND/VCC)
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
      Serial.printf("WiFi OK, IP: %s\n", WiFi.localIP().toString().c_str());
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
  fields.set("ts/timestampValue", isoTs);
  fields.set("dev/stringValue", "esp32_01");
  content.set("fields", fields);

  String mask = "soil_mv,soil_pct,ldr_mv,ph,npk_n,npk_p,npk_k,pump_on,ts,dev";
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
  delay(300);
  Serial.println("\n🌱 Start: solo + NPK(botões) + pH(LDR) + bomba(relé) + Open-Meteo");

  analogSetPinAttenuation(SOIL_PIN, ADC_11db);
  analogSetPinAttenuation(LDR_PIN,  ADC_11db);

  pinMode(BTN_N, INPUT_PULLUP);
  pinMode(BTN_P, INPUT_PULLUP);
  pinMode(BTN_K, INPUT_PULLUP);

  pinMode(RELAY_PIN, OUTPUT);
  relayWrite(false);   // garante desligado
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

  // 1) Leituras periódicas
  if (millis() - last_csv_ms >= CSV_PERIOD_MS) {
    last_csv_ms = millis();

    // Solo
    int soil_mv  = medianMvPin(SOIL_PIN);
    int soil_pct = constrain(map(soil_mv, MV_DRY, MV_WET, 0, 100), 0, 100);

    // ===== Controle da bomba (histerese + mínimos + segurança) =====
    unsigned long now = millis();
    bool armed     = (now - bootMs) >= ARM_DELAY_MS;
    bool canOn     = (!pumpOn) && (now - pumpLastChangeMs >= MIN_OFF_MS);
    bool canOff    = ( pumpOn) && (now - pumpLastChangeMs >= MIN_ON_MS);
    bool maxOnHit  = ( pumpOn) && (now - pumpLastChangeMs >= MAX_ON_MS);

    if (maxOnHit && canOff) {
      Serial.println("[SAFE] Tempo máximo ligado atingido. Desligando.");
      relayWrite(false);
    } else if (armed) {
      if (!pumpOn && soil_pct < SOIL_ON_PCT && canOn)          relayWrite(true);
      else if (pumpOn && soil_pct >= SOIL_OFF_PCT && canOff)   relayWrite(false);
    }

    // LDR → pH
    int   ldr_mv = readLDRmVComp();
    float ph     = fmap((float)ldr_mv, (float)LDR_MV_MIN_DARK, (float)LDR_MV_MAX_BRIGHT, 0.0f, (float)PH_MAX);
    ph = constrain(ph, 0.0f, (float)PH_MAX);

    // Timestamp
    String ts = ntp_ready ? iso8601NowUTC() : String("1970-01-01T00:00:00Z");

    // Logs locais
    Serial.printf("SOLO mV=%4d Umid=%3d%% | LDR mV=%4d pH=%.2f | N=%d P=%d K=%d | PUMP=%d | %s\n",
                  soil_mv, soil_pct, ldr_mv, ph, npkLevelN, npkLevelP, npkLevelK, pumpOn ? 1 : 0, ts.c_str());
    Serial.printf("CSV,%s,%d,%d,%d,%.2f,%d,%d,%d,%d\n",
                  ts.c_str(), soil_mv, soil_pct, ldr_mv, ph, npkLevelN, npkLevelP, npkLevelK, pumpOn ? 1 : 0);

    // Firestore (leituras)
    if (wifi_connected && fb_ready && (millis() - last_fs_ms >= FS_PERIOD_MS)) {
      last_fs_ms = millis();
      if (writeToFirestore(soil_mv, soil_pct, ldr_mv, ph, npkLevelN, npkLevelP, npkLevelK, pumpOn, ts))
        Serial.println("[Firestore] OK");
      else
        Serial.println("[Firestore] Falhou");
    }
  }

  // 2) Meteo: após CSV para não bloquear leituras
  if (wifi_connected && (millis() - last_weather_ms >= WEATHER_PERIOD_MS)) {
    last_weather_ms = millis();
    if (fetchWeather()) {
      String ts = ntp_ready ? iso8601NowUTC() : String("1970-01-01T00:00:00Z");
      String popStr = isnan(wx_pop3h) ? String("NA") : String(wx_pop3h*100.0f, 0);
      Serial.printf("OM{T=%.1fC POP3h=%s%% PR3h=%.1fmm code=%d %s}\n",
                    wx_tempC, popStr.c_str(), (isnan(wx_rain3h)?0.0f:wx_rain3h), wx_code, wx_desc.c_str());

      // Linha compacta com as 3 próximas horas (POP/PR)
      Serial.printf("OM.h [%s] POP=%.0f%% PR=%.1fmm | [%s] POP=%.0f%% PR=%.1fmm | [%s] POP=%.0f%% PR=%.1fmm\n",
                    wx_time_h[0].c_str(), (isnan(wx_pop_h[0])?0.0f:wx_pop_h[0]), (isnan(wx_precip_h[0])?0.0f:wx_precip_h[0]),
                    wx_time_h[1].c_str(), (isnan(wx_pop_h[1])?0.0f:wx_pop_h[1]), (isnan(wx_precip_h[1])?0.0f:wx_precip_h[1]),
                    wx_time_h[2].c_str(), (isnan(wx_pop_h[2])?0.0f:wx_pop_h[2]), (isnan(wx_precip_h[2])?0.0f:wx_precip_h[2]));

      if (fb_ready) {
        if (!writeWeatherToFirestore(ts)) Serial.println("[Firestore] weather: falhou");
      }
    }
  }

  // 3) Wi-Fi / NTP / FB
  if (WiFi.status() == WL_CONNECTED) wifi_connected = true; else { wifi_connected = false; tryConnectWiFi(); }
  trySetupTime();
  tryInitFirebase();

  delay(5);
}
