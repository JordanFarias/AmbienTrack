#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define DHTPIN 15
#define DHTTYPE DHT22
#define LDR_PIN 34
#define MQ2_PIN 35
#define PIN_VENTILADOR 25
#define PIN_ALARMA 26

#ifndef WIFI_SSID
#define WIFI_SSID "Wokwi-GUEST"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif
#ifndef FIREBASE_API_KEY
#define FIREBASE_API_KEY "AIzaSyD_9anc_Uaei2iGVISoqBVZ9Y36001sX5A"
#endif
#ifndef FIREBASE_DB_URL
#define FIREBASE_DB_URL "https://estacion-ambiental-5b4b1-default-rtdb.firebaseio.com/"
#endif

DHT dht(DHTPIN, DHTTYPE);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastUpdate = 0;
const unsigned long INTERVALO = 3000;,

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_VENTILADOR, OUTPUT);
    pinMode(PIN_ALARMA, OUTPUT);
    digitalWrite(PIN_VENTILADOR, LOW);
    digitalWrite(PIN_ALARMA, LOW);

    dht.begin();
    Serial.println("\n[BOOT] 1/4 Periféricos OK");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[BOOT] 2/4 Conectando Wi-Fi");
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 30) {
        delay(300);
        Serial.print(".");
        retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[BOOT] 2/4 Wi-Fi OK (IP: %s)\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[BOOT] Error: Wi-Fi no conectó");
    }

    // Configuración directa para reglas publicas / secreto de base de datos
    config.api_key = FIREBASE_API_KEY;
    config.database_url = FIREBASE_DB_URL;
    config.signer.tokens.legacy_token = "KoTlVPQ4tbAkzhu1XJn6KUNEYVxzz2EiVqTUUWBJ";

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    Serial.println("[BOOT] 3/4 Firebase conectado");
    Serial.println("[BOOT] 4/4 Setup completo, entrando a loop");
}

void loop() {
    // Alimenta el Watchdog Timer
    delay(20);

    if (!Firebase.ready()) return;

    // 1. Polling de actuadores desde Firebase
    if (Firebase.RTDB.getBool(&fbdo, "/ventilador_estado")) {
        digitalWrite(PIN_VENTILADOR, fbdo.boolData() ? HIGH : LOW);
    }
    if (Firebase.RTDB.getBool(&fbdo, "/alarma_estado")) {
        digitalWrite(PIN_ALARMA, fbdo.boolData() ? HIGH : LOW);
    }

    // 2. Envío de telemetría cada 3 segundos
    if (millis() - lastUpdate >= INTERVALO) {
        lastUpdate = millis();

        float h = dht.readHumidity();
        float t = dht.readTemperature();
        int luz = analogRead(LDR_PIN);
        int gas = analogRead(MQ2_PIN);

        if (isnan(h) || isnan(t)) return;

        int confort = 100;
        if (t < 20 || t > 26) confort -= 20;
        if (h < 40 || h > 60) confort -= 20;
        if (luz < 500) confort -= 15;
        if (gas > 1500) confort -= 45;
        if (confort < 0) confort = 0;

        Serial.printf("[TX] Temp: %.1f | Hum: %.1f | Luz: %d | Gas: %d | Confort: %d\n", t, h, luz, gas, confort);

        Firebase.RTDB.setDouble(&fbdo, "/Temperatura", t);
        Firebase.RTDB.setDouble(&fbdo, "/Humedad", h);
        Firebase.RTDB.setInt(&fbdo, "/Luz", luz);
        Firebase.RTDB.setInt(&fbdo, "/Gas", gas);
        Firebase.RTDB.setInt(&fbdo, "/Confort", confort);
    }
}