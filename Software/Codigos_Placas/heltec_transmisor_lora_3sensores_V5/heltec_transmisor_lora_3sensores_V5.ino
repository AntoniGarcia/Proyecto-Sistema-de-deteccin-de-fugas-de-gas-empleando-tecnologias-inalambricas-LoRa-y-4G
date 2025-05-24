#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"  // Librería para la pantalla OLED

// Configuración de la pantalla OLED
SSD1306Wire factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// Configuración LoRa
#define RF_FREQUENCY        915000000 // Hz (ajustar según región)
#define TX_OUTPUT_POWER     5         // Potencia de transmisión en dBm
#define LORA_BANDWIDTH      0         // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz]
#define LORA_SPREADING_FACTOR 7       // [SF7..SF12]
#define LORA_CODINGRATE     1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH 8        
#define LORA_SYMBOL_TIMEOUT  0        
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 128 // Tamaño del paquete LoRa (aumentado para incluir más datos)

char txpacket[BUFFER_SIZE];  // Buffer de datos a enviar
bool lora_idle = true;

static RadioEvents_t RadioEvents;

// Prototipos de funciones
void OnTxDone(void);
void OnTxTimeout(void);

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup() {
    Serial.begin(115200);  // Para depuración
    Serial2.begin(115200, SERIAL_8N1, 43, 44); // Comunicación UART con Arduino (RX=43, TX=44)

    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    // Encender la pantalla OLED
    VextON();
    delay(100);
    factory_display.init();
    factory_display.clear();
    factory_display.display();

    // Configurar eventos de transmisión
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;

    // Inicializar LoRa
    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);
    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
}

void loop() {
    if (Serial2.available()) {
        // Leer datos del Arduino
        String receivedData = Serial2.readStringUntil('\n');
        
        int gasIndex = receivedData.indexOf("\"gas\":");
        int pressureIndex = receivedData.indexOf("\"presion\":");
        int tempIndex = receivedData.indexOf("\"temp\":");

        String gasValue = receivedData.substring(gasIndex + 6, receivedData.indexOf(",", gasIndex));
        String pressureValue = receivedData.substring(pressureIndex + 10, receivedData.indexOf(",", pressureIndex));
        String tempValue = receivedData.substring(tempIndex + 7, receivedData.indexOf("}", tempIndex));

        // Formatear y preparar el paquete LoRa
        snprintf(txpacket, BUFFER_SIZE, "gas: %s V, presión: %s kPa,  temp: %s", 
                 gasValue.c_str(),pressureValue.c_str(),tempValue.c_str());

        Serial.printf("📡 Enviando por LoRa: %s\n", txpacket);

        // Mostrar datos en la pantalla OLED
        factory_display.clear();
        factory_display.setFont(ArialMT_Plain_10);
        factory_display.setTextAlignment(TEXT_ALIGN_CENTER);
        factory_display.drawString(64, 0, "Datos del Sensor");
        factory_display.setTextAlignment(TEXT_ALIGN_LEFT);
        factory_display.drawString(0, 10, "Gas: " + gasValue + " V");
        factory_display.drawString(0, 30, "Presión: " + pressureValue + " kPa");
        factory_display.drawString(0, 20, "Temp: " + tempValue);
        factory_display.display();

        // Enviar datos por LoRa si el canal está libre
        if (lora_idle) {
            Radio.Send((uint8_t *)txpacket, strlen(txpacket));
            lora_idle = false;
        }
    }
    Radio.IrqProcess();
}

// Función llamada cuando la transmisión LoRa se completa
void OnTxDone(void) {
    Serial.println("✅ Transmisión LoRa completada.");
    lora_idle = true;
}

// Función en caso de error en la transmisión
void OnTxTimeout(void) {
    Radio.Sleep();
    Serial.println("❌ Error: Timeout en transmisión LoRa.");
    lora_idle = true;
}