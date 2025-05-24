#include <OneWire.h>
#include <DallasTemperature.h>
#include <MAX6675.h>

// Pines del sensor MQ-6 y presión
#define MQ6_PIN A0
#define PRESSURE_PIN A1
#define ONE_WIRE_BUS 2

// Pines del termopar MAX6675
int thermoDO = 10;
int thermoCS = 9;
int thermoCLK = 8;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// Constantes del sensor de presión
const float VCC = 5.0;
const float VOLTAGE_MIN = 0.5;
const float VOLTAGE_MAX = 4.5;
const float PRESSURE_MIN = 0.0;
const float PRESSURE_MAX = 100.0;

// Inicialización del sensor DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Variables MQ-6
#define RL 1000.0
#define RO_CLEAN_AIR_FACTOR 10.0
float Ro;
float m = -0.45;
float b = 1.0;

void setup() {
  Serial.begin(115200);
  sensors.begin();
  Ro = calibrarSensor();

  pinMode(thermoCLK, OUTPUT);
  pinMode(thermoCS, OUTPUT);
  pinMode(thermoDO, INPUT);
  digitalWrite(thermoCS, HIGH);
  digitalWrite(thermoCLK, LOW);
}

void loop() {
  // MQ-6
  float sensorValue = analogRead(MQ6_PIN);
  float sensorVolt = sensorValue * (VCC / 1023.0);
  float RS_gas = ((VCC * RL) / sensorVolt) - RL;
  float ratio = RS_gas / Ro;
  float ppm = pow(10, ((log10(ratio) - b) / m));

  // Presión
  int pressureValue = analogRead(PRESSURE_PIN);
  float pressureVoltage = pressureValue * (VCC / 1023.0);
  float pressure = (pressureVoltage - VOLTAGE_MIN) * (PRESSURE_MAX - PRESSURE_MIN) / (VOLTAGE_MAX - VOLTAGE_MIN) + PRESSURE_MIN;

  // Temperatura (usando MAX6675)
  float temperatureC = readThermocouple();

  // Crear JSON manualmente
  Serial.print("{\"gas\":");
  Serial.print((int)ppm);
  Serial.print(",\"presion\":");
  Serial.print(pressure, 2);
  Serial.print(",\"temp\":");
  Serial.print(temperatureC, 2);
  Serial.println("}");

  delay(1000);
}

// Calibrar MQ-6 en aire limpio
float calibrarSensor() {
  long suma = 0;
  const int numLecturas = 50;
  for (int i = 0; i < numLecturas; i++) {
    suma += analogRead(MQ6_PIN);
    delay(100);
  }
  float avg = suma / (float)numLecturas;
  float sensorVolt = avg * (VCC / 1023.0);
  float RS_air = ((VCC * RL) / sensorVolt) - RL;
  return RS_air / RO_CLEAN_AIR_FACTOR;
}

// Leer temperatura desde MAX6675
float readThermocouple() {
  uint16_t data = 0;
  digitalWrite(thermoCS, LOW);
  delayMicroseconds(10);
  for (int i = 15; i >= 0; i--) {
    digitalWrite(thermoCLK, HIGH);
    delayMicroseconds(10);
    data |= digitalRead(thermoDO) << i;
    digitalWrite(thermoCLK, LOW);
    delayMicroseconds(10);
  }
  digitalWrite(thermoCS, HIGH);
  if (data & 0x4) return NAN;
  return (data >> 3) * 0.25;
}
