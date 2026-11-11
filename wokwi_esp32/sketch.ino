/*
 * AmbienTrack - Firmware ESP32 (Wokwi / PlatformIO)
 * ESTABLE: polling simple, sin streams ni tareas RTOS propias.
 *
 * Publica (sensores, cada 3 s): Temperatura, Humedad, Luz, Gas, Confort
 * Lee (actuadores, cada 1 s):   ventilador_estado, alarma_estado (getBool)
 */

#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"

// --- Configuración de pines ---
#define DHTPIN 15        // Pin digital del DHT22
#define DHTTYPE DHT22
#define LDR_PIN 34       // Pin analógico del LDR (salida AO)
#define MQ2_PIN 35       // Pin analógico del MQ-2 (salida AO)
#define PIN_VENTILADOR 25 // Relé / LED del ventilador (actuador)
#define PIN_ALARMA 26     // Buzzer / alarma (actuador)

// --- Credenciales ---
#ifndef WIFI_SSID
#define WIFI_SSID "TU_SSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "TU_PASSWORD"
#endif
#ifndef FIREBASE_API_KEY
#define FIREBASE_API_KEY "TU_API_KEY_WEB"
#endif
#ifndef FIREBASE_DB_SECRET
#define FIREBASE_DB_SECRET ""
#endif
#ifndef FIREBASE_URL
#define FIREBASE_URL "https://estacion-ambiental-5b4b1-default-rtdb.firebaseio.com/"
#endif

// --- Constantes ---
const unsigned long WIFI_TIMEOUT_MS = 15000;
const unsigned long WIFI_RETRY_MS = 10000;
const unsigned long PUBLICAR_MS = 3000;
const unsigned long POLL_MS = 1000;

DHT dht(DHTPIN, DHTTYPE);
FirebaseData fbWrite;    // Publicación de telemetría
FirebaseData fbRead;     // Polling de actuadores
FirebaseAuth auth;
FirebaseConfig config;

bool ventOn = false;
bool alarmaOn = false;
bool firebaseReady = false;
unsigned long lastPublish = 0;
unsigned long lastPoll = 0;
unsigned long lastWifiRetry = 0;

// --- Validación de credenciales ---
bool credencialReal(const char *v) {
  if (v == nullptr || strlen(v) == 0) return false;
  return strcmp(v, "TU_SSID") != 0 && strcmp(v, "TU_PASSWORD") != 0 &&
         strcmp(v, "TU_API_KEY_WEB") != 0 && strcmp(v, "TU_DATABASE_SECRET") != 0;
}

bool wifiConfigurado() {
  return credencialReal(WIFI_SSID) && credencialReal(WIFI_PASSWORD);
}

bool firebaseConfigurado() {
  return credencialReal(FIREBASE_URL) && strncmp(FIREBASE_URL, "https://", 8) == 0 &&
         credencialReal(FIREBASE_API_KEY);
}

bool conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  if (!wifiConfigurado()) return false;

  Serial.printf("[BOOT] Conectando a Wi-Fi \"%s\"", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < WIFI_TIMEOUT_MS) {
    Serial.print(".");
    delay(10);
    yield(); // alimenta el WDT
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf(" OK. IP: %s\r\n", WiFi.localIP().toString().c_str());
    return true;
  }
  Serial.println(" FALLÓ (timeout)");
  WiFi.disconnect(false);
  return false;
}

int calcularIndiceConfort(float temp, float hum, int luz, int gas) {
  int indice = 100;

  // Penaliza si la temperatura se aleja de un rango ideal (20-26°C)
  if (temp < 20 || temp > 26) indice -= 20;

  // Penaliza si la humedad se aleja de un rango ideal (40-60%)
  if (hum < 40 || hum > 60) indice -= 20;

  // Penaliza si hay poca luz (ambiente cerrado, sin ventilar)
  if (luz < 500) indice -= 15;

  // Penaliza fuerte si el gas está elevado (lo más crítico)
  if (gas > 1500) indice -= 45;

  if (indice < 0) indice = 0;
  return indice;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("[BOOT] 1/4 Periféricos");
  dht.begin();

  pinMode(PIN_VENTILADOR, OUTPUT);
  pinMode(PIN_ALARMA, OUTPUT);
  digitalWrite(PIN_VENTILADOR, LOW);
  digitalWrite(PIN_ALARMA, LOW);

  Serial.println("[BOOT] 2/4 Wi-Fi");
  bool wifiOK = conectarWiFi();
  lastWifiRetry = millis();

  firebaseReady = wifiOK && firebaseConfigurado();
  if (!firebaseConfigurado()) {
    Serial.println("[BOOT] AVISO: credenciales Firebase inválidas -> modo local (Serial)");
  }

  if (firebaseReady) {
    Serial.println("[BOOT] 3/4 Config Firebase");
    config.api_key = FIREBASE_API_KEY;
    config.database_url = FIREBASE_URL;
    config.token_status_callback = tokenStatusCallback;

    if (strlen(FIREBASE_DB_SECRET) > 0) {
      // Autenticación por token legado
      config.signer.tokens.legacy_token = FIREBASE_DB_SECRET;
      Serial.println("[BOOT] 3/4 Auth: token legado");
    } else {
      // Autenticación anonima con API Key del proyecto
      if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("[BOOT] 3/4 Auth: anónima OK");
      } else {
        Serial.printf("[BOOT] 3/4 Auth anónima falló: %s\r\n",
                      config.signer.signupError.message.c_str());
        firebaseReady = false;
      }
    }

    if (firebaseReady) {
      Serial.println("[BOOT] 4/4 Firebase.begin");
      Firebase.begin(&config, &auth);
      Firebase.reconnectWiFi(true);
    }
  } else {
    Serial.println("[BOOT] Firebase diferido (sin Wi-Fi o credenciales inválidas)");
  }

  Serial.println("[BOOT] setup completo, entrando a loop");
}

void loop() {
  // Reintento de Wi-Fi + Firebase sin bloquear el loop
  if (WiFi.status() != WL_CONNECTED && millis() - lastWifiRetry > WIFI_RETRY_MS) {
    lastWifiRetry = millis();
    if (conectarWiFi() && firebaseConfigurado() && !firebaseReady) {
      firebaseReady = true;
      config.api_key = FIREBASE_API_KEY;
      config.database_url = FIREBASE_URL;
      config.token_status_callback = tokenStatusCallback;
      if (strlen(FIREBASE_DB_SECRET) > 0) {
        config.signer.tokens.legacy_token = FIREBASE_DB_SECRET;
      }
      Firebase.begin(&config, &auth);
      Firebase.reconnectWiFi(true);
    }
  }

  // --- Polling de actuadores (cada 1 s, sin streams ni callbacks) ---
  if (firebaseReady && Firebase.ready() && millis() - lastPoll >= POLL_MS) {
    lastPoll = millis();

    bool v = false;
    if (Firebase.RTDB.getBool(&fbRead, "/ventilador_estado", &v)) {
      if (v != ventOn) {
        ventOn = v;
        digitalWrite(PIN_VENTILADOR, ventOn ? HIGH : LOW);
        Serial.printf("[POLL] ventilador_estado = %s\r\n", ventOn ? "ON" : "OFF");
      }
    }

    bool a = false;
    if (Firebase.RTDB.getBool(&fbRead, "/alarma_estado", &a)) {
      if (a != alarmaOn) {
        alarmaOn = a;
        digitalWrite(PIN_ALARMA, alarmaOn ? HIGH : LOW);
        Serial.printf("[POLL] alarma_estado = %s\r\n", alarmaOn ? "ON" : "OFF");
      }
    }
  }

  // --- Publicación de telemetria (cada 3 s) ---
  if (firebaseReady && Firebase.ready() && millis() - lastPublish >= PUBLICAR_MS) {
    lastPublish = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int luz = analogRead(LDR_PIN);
    int gas = analogRead(MQ2_PIN);

    if (isnan(h) || isnan(t)) {
      Serial.println("Error leyendo el DHT22");
    } else {
      int indice = calcularIndiceConfort(t, h, luz, gas);

      Serial.println("----------------------------");
      Serial.printf("Temperatura: %.1f°C\r\n", t);
      Serial.printf("Humedad: %.1f%%\r\n", h);
      Serial.printf("Luz (crudo): %d\r\n", luz);
      Serial.printf("Gas (crudo): %d\r\n", gas);
      Serial.printf("Índice de confort: %d/100\r\n", indice);
      Serial.printf("Ventilador: %s | Alarma: %s\r\n", ventOn ? "ON" : "OFF",
                    alarmaOn ? "ON" : "OFF");

      Firebase.RTDB.setDouble(&fbWrite, "/Temperatura", t);
      Firebase.RTDB.setDouble(&fbWrite, "/Humedad", h);
      Firebase.RTDB.setInt(&fbWrite, "/Luz", luz);
      Firebase.RTDB.setInt(&fbWrite, "/Gas", gas);
      Firebase.RTDB.setInt(&fbWrite, "/Confort", indice);
    }
  }

  delay(10); // alimenta el Watchdog Timer SIEMPRE
  yield();
}
