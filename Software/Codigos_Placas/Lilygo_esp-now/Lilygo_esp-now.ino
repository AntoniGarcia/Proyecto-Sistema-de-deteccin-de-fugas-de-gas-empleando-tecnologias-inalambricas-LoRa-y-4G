#define TINY_GSM_MODEM_SIM7600
#define SerialMon Serial

#ifndef _AVR_ATmega328P_
  #define SerialAT Serial1
#else
  #include <SoftwareSerial.h>
  SoftwareSerial SerialAT(2, 3);
#endif

#define TINY_GSM_DEBUG SerialMon
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
const char apn[] = "internet.itelcel.com";
const char gprsUser[] = "webgprs";
const char gprsPass[] = "webgprs2002";

// MQTT
const char* broker = "broker.hivemq.com";
const char* topicData = "GsmClientTest/data";

#include <esp_now.h>
#include <WiFi.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

uint32_t lastReconnectAttempt = 0;
bool modemInfoPrinted = false;

#define LED_PIN 12
String jsonData = "";

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Mensaje recibido [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();
}

boolean mqttConnect() {
  SerialMon.print("Conectando a ");
  SerialMon.print(broker);
  boolean status = mqtt.connect("ESP32SIMClient");
  if (!status) {
    SerialMon.println(" fallo");
    return false;
  }
  SerialMon.println(" éxito");
  mqtt.subscribe(topicData);
  return mqtt.connected();
}

void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
           recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
  SerialMon.print("Datos recibidos de: ");
  SerialMon.println(macStr);

  char buffer[len + 1];
  memcpy(buffer, incomingData, len);
  buffer[len] = '\0';

  SerialMon.print("Datos JSON: ");
  SerialMon.println(buffer);

  jsonData = String(buffer);
}

void setup() {
  SerialMon.begin(115200);
  delay(10);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  delay(1000);
  digitalWrite(4, LOW);

  SerialMon.println("Inicializando módem...");
  SerialAT.begin(115200);
  delay(6000);
  modem.restart();

  if (modem.getSimStatus() != SIM_READY) {
    SerialMon.println("¡Tarjeta SIM no detectada!");
    return;
  }

  if (!modem.waitForNetwork()) {
    SerialMon.println("No se encontró red");
    return;
  }

  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println("Fallo en conexión GPRS");
    return;
  }

  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);

  // Inicializar ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    SerialMon.println("Error iniciando ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (!modem.isNetworkConnected() || !modem.isGprsConnected()) {
    modem.gprsConnect(apn, gprsUser, gprsPass);
  }

  if (!mqtt.connected()) {
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqtt.loop();
  }

  // Si se recibió un JSON, publícalo
  if (jsonData.length() > 0) {
    mqtt.publish(topicData, jsonData.c_str());
    SerialMon.println("Publicado en MQTT: " + jsonData);
    jsonData = "";
  }
}
