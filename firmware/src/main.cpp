#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>
#include "env.h"

// ========= SENSOR =========
#define SOIL_PIN 32
const int MV_DRY = 3181;
const int MV_WET = 2300;

// ========= FIREBASE OBJ =========
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ========= ESTADO =========
bool wifi_connected = false;
bool ntp_ready = false;
bool fb_ready = false;

unsigned long last_wifi_try_ms = 0;
unsigned long last_ntp_check_ms = 0;
unsigned long last_csv_ms = 0;
unsigned long last_fs_ms  = 0;

const unsigned long WIFI_RETRY_MS   = 10000;  // 10s
const unsigned long CSV_PERIOD_MS   = 2000;   // 2s
const unsigned long FS_PERIOD_MS    = 15000;  // 15s
const unsigned long NTP_TIMEOUT_MS  = 10000;  // 10s

// ========= LEITURA COM MEDIANA =========
int medianMv(int n = 21) {
  int v[41];
  if (n > 41) n = 41;
  for (int i = 0; i < n; i++) { v[i] = analogReadMilliVolts(SOIL_PIN); delay(2); }
  for (int i = 1; i < n; i++) {
    int x = v[i], j = i - 1;
    while (j >= 0 && v[j] > x) { v[j + 1] = v[j]; j--; }
    v[j + 1] = x;
  }
  return v[n / 2];
}

// ========= TEMPO =========
String iso8601NowUTC() {
  time_t now; time(&now);
  struct tm tm_utc; gmtime_r(&now, &tm_utc);
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
  return String(buf);
}

// ========= WIFI (não-bloqueante) =========
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

  if (!wifi_connected) {
    Serial.printf("Conectando WiFi SSID='%s'...\n", WIFI_SSID);
  } else {
    Serial.println("WiFi caiu, tentando reconectar...");
  }

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// ========= NTP (após WiFi) =========
void trySetupTime() {
  if (ntp_ready || !wifi_connected) return;

  if (last_ntp_check_ms == 0) {
    Serial.println("Sincronizando NTP...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    last_ntp_check_ms = millis();
    return;
  }

  time_t now; time(&now);
  struct tm tm_info; gmtime_r(&now, &tm_info);
  if (tm_info.tm_year > (2016 - 1900)) {
    ntp_ready = true;
    Serial.println("NTP OK");
  } else if (millis() - last_ntp_check_ms > NTP_TIMEOUT_MS) {
    Serial.println("[WARN] NTP timeout, seguindo sem NTP (tentará de novo depois).");
    ntp_ready = false;
    last_ntp_check_ms = 0; // permitirá nova tentativa depois
  }
}

// ========= FIREBASE =========
void tryInitFirebase() {
  if (fb_ready || !wifi_connected) return;

  Serial.println("Autenticando Firebase...");
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);

  unsigned long t0 = millis();
  while (auth.token.uid == "") {
    if (millis() - t0 > 15000) {
      Serial.println("[ERRO] Timeout autenticando Firebase (tentará depois).");
      return; // sai, tenta novamente mais tarde
    }
    delay(300);
    Serial.print(".");
  }
  Serial.printf("\nFirebase OK. UID: %s\n", auth.token.uid.c_str());
  fb_ready = true;
}

// ========= FIRESTORE: último + histórico =========
bool writeToFirestore(int mv, int pct, const String& isoTs) {
  FirebaseJson content, fields;
  fields.set("mv/integerValue", mv);
  fields.set("pct/integerValue", pct);
  fields.set("ts/timestampValue", isoTs);
  fields.set("dev/stringValue", "esp32_01");
  content.set("fields", fields);

  bool ok = false;

  // 1) upsert último valor
  String mask = "mv,pct,ts,dev";
  ok = Firebase.Firestore.patchDocument(&fbdo, PROJECT_ID, FIRESTORE_DB,
                                        "leituras/esp32_01", content.raw(), mask.c_str());
  if (!ok) {
    Serial.printf("[Firestore] patch erro: %s\n", fbdo.errorReason().c_str());
    ok = Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, FIRESTORE_DB,
                                           "leituras/esp32_01", content.raw());
    if (!ok) Serial.printf("[Firestore] create erro: %s\n", fbdo.errorReason().c_str());
  }

  // 2) append histórico
  bool ok_hist = Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, FIRESTORE_DB,
                                                   "leituras/esp32_01/historico", content.raw());
  if (!ok_hist) Serial.printf("[Firestore] historico erro: %s\n", fbdo.errorReason().c_str());

  return ok && ok_hist;
}

// ========= SETUP =========
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  delay(500);
  Serial.println("\n🌱 Monitorando umidade do solo (non-blocking WiFi/Firebase)");

  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  last_wifi_try_ms = 0;   // força tentar já
  tryConnectWiFi();
}

// ========= LOOP =========
void loop() {
  // 1) CSV sempre
  if (millis() - last_csv_ms >= CSV_PERIOD_MS) {
    last_csv_ms = millis();

    int mv  = medianMv();
    int pct = constrain(map(mv, MV_DRY, MV_WET, 0, 100), 0, 100);
    String ts = iso8601NowUTC();

    Serial.printf("mV=%4d  Umidade=%3d%%  ts=%s\n", mv, pct, ts.c_str());
    Serial.printf("CSV,%s,%d,%d\n", ts.c_str(), mv, pct);

    // 2) Firestore quando possível e no período
    if (wifi_connected && fb_ready && (millis() - last_fs_ms >= FS_PERIOD_MS)) {
      last_fs_ms = millis();
      if (writeToFirestore(mv, pct, ts)) Serial.println("[Firestore] OK");
      else Serial.println("[Firestore] Falhou");
    }
  }

  // 3) Wi-Fi watchdog/retry
  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
  } else {
    wifi_connected = false;
    tryConnectWiFi();
  }

  // 4) NTP/Firebase após Wi-Fi
  trySetupTime();
  tryInitFirebase();

  delay(5);
}
