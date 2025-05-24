#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <esp_now.h>
#include <WiFi.h>

// Dirección MAC del receptor ESP-NOW (modifica esto con la dirección real)
uint8_t peer_addr[] = {0xC8, 0x2E, 0x18, 0x6B, 0xF9, 0x70}; // <- CAMBIA ESTO

SSD1306Wire factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// Configuración LoRa
#define RF_FREQUENCY        915000000
#define TX_OUTPUT_POWER     14
#define LORA_BANDWIDTH      0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE     1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT  0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 128

char rxpacket[BUFFER_SIZE];
bool lora_idle = true;

static RadioEvents_t RadioEvents;
int16_t rssi, rxSize;

// Declaración anticipada
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

// ESP-NOW callback
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("ESP-NOW Envío de datos: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "✅ Éxito" : "❌ Falló");
}

// Inicialización de ESP-NOW
void initESPNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ Error inicializando ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peer_addr, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("❌ No se pudo agregar el peer");
  } else {
    Serial.println("✅ Peer agregado");
  }
}

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  WiFi.mode(WIFI_STA);  // Importante para ESP-NOW
  initESPNow();

  VextON();
  delay(100);
  factory_display.init();
  factory_display.clear();
  factory_display.display();

  RadioEvents.RxDone = OnRxDone;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
}

void loop() {
  if (lora_idle) {
    lora_idle = false;
    Serial.println("Esperando paquetes LoRa...");
    Radio.Rx(0);
  }
  Radio.IrqProcess();
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  rxSize = size;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  Radio.Sleep();

  Serial.printf("\n📡 Paquete recibido: \"%s\" con RSSI: %d, Tamaño: %d bytes\n", rxpacket, rssi, rxSize);

  String receivedData = String(rxpacket);
  receivedData.trim();

  // Extraer valores
  int gasIndex = receivedData.indexOf("gas:");
  int gasEnd = receivedData.indexOf("V", gasIndex);
  String gasValue = receivedData.substring(gasIndex + 5, gasEnd);
  gasValue.trim();

  int presionIndex = receivedData.indexOf("presión:");
  int presionEnd = receivedData.indexOf("kPa", presionIndex);
  String pressureValue = receivedData.substring(presionIndex + 9, presionEnd);
  pressureValue.trim();

  int tempIndex = receivedData.indexOf("temp:");
  String tempValue = receivedData.substring(tempIndex + 6);
  tempValue.trim();

  // Mostrar en pantalla OLED
  factory_display.clear();
  factory_display.setFont(ArialMT_Plain_10);
  factory_display.setTextAlignment(TEXT_ALIGN_CENTER);
  factory_display.drawString(64, 0, "Datos del Sensor");
  factory_display.setTextAlignment(TEXT_ALIGN_LEFT);
  factory_display.drawString(0, 10, "Gas: " + gasValue + " V");
  factory_display.drawString(0, 20, "Temp: " + tempValue + " C");
  factory_display.drawString(0, 30, "Presión: " + pressureValue + " kPa");
  factory_display.drawString(0, 40, "RSSI: " + String(rssi) + " dBm");
  factory_display.display();

  // Formato JSON manual (como cadena)
  String jsonString = "{\"gas\":\"" + gasValue + "\",\"presion\":\"" + pressureValue + "\",\"temperatura\":\"" + tempValue + "\"}";
  Serial.println("📤 Enviando por ESP-NOW: " + jsonString);

  // Envío
  esp_err_t result = esp_now_send(peer_addr, (uint8_t *)jsonString.c_str(), jsonString.length());
  if (result != ESP_OK) {
    Serial.println("❌ Error al enviar el paquete por ESP-NOW");
  }

  lora_idle = true;
}
