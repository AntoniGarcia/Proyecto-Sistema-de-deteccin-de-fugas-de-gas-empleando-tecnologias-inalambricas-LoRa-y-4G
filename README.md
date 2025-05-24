
# Sistema de Detección de Fugas de Gas LP en Viviendas 🏠🔥

Este proyecto tiene como objetivo la **prevención de accidentes domésticos** mediante la detección temprana de fugas de gas LP, reduciendo el riesgo de incendios o explosiones. El sistema emplea sensores ambientales y tecnologías de comunicación inalámbrica **LoRa** y **4G** para la transmisión remota de datos en tiempo real hacia una plataforma de monitoreo.

---

## 🖐️ Arquitectura del Sistema

El sistema se compone de varias etapas, cada una con una función específica dentro del proceso de captura, transmisión y visualización de los datos.

### 🔹 Arduino Uno
- **Función:** Lectura de sensores de gas, presión y temperatura.
- **Salida de datos:** Objeto JSON con la siguiente estructura:
  ```json
  {"gas":127,"presion":52.38,"temp":24.50}
  ```
- **Comunicación:** Transmisión de datos mediante **UART** hacia la placa Heltec LoRa 32 V3.

### 🔹 Heltec LoRa 32 V3 – Nodo 1
- **Función:** Recepción del JSON vía UART, visualización en pantalla OLED, y envío inalámbrico mediante **LoRa** a un segundo nodo Heltec.

### 🔹 Heltec LoRa 32 V3 – Nodo 2
- **Función:** Recepción de datos LoRa, visualización local, y retransmisión usando **ESP-NOW** hacia una placa LilyGO TTGO SIM7600.

### 🔹 LilyGO TTGO SIM7600
- **Función:** Recepción del JSON vía ESP-NOW y posterior envío a internet mediante red móvil 3G.
- **Protocolo utilizado:** **MQTT**, hacia el broker público [HiveMQ](https://www.hivemq.com/).

### 🔹 Node-RED
- **Función:** Suscripción al tópico MQTT, visualización de datos en tiempo real a través de dashboards personalizados.

### 🔹 Raspberry Pi
- **Función:** Actúa como servidor local. Se encarga del almacenamiento, visualización y gestión de los datos del sistema.

---

## 🔗 Protocolos de Comunicación

| Protocolo | Descripción |
|----------|-------------|
| **UART** | Comunicación serial entre Arduino y Nodo LoRa 1 mediante pines TX/RX. |
| **LoRa** | Comunicación inalámbrica de largo alcance y bajo consumo entre nodos Heltec. |
| **ESP-NOW** | Protocolo de baja latencia entre dispositivos ESP32, usado entre Nodo 2 y LilyGO. |
| **MQTT** | Protocolo ligero y eficiente para IoT, utilizado para enviar datos al broker en la nube. |

---

## 🛠️ Tecnologías Empleadas

- Arduino Uno  
- Heltec LoRa 32 V3 (x2)  
- LilyGO TTGO SIM7600  
- Raspberry Pi  
- Node-RED  
- HiveMQ  
- Sensores de gas, presión y temperatura  

---

## 📊 Visualización de Datos

El sistema proporciona visualización en tiempo real mediante **dashboards construidos en Node-RED**, permitiendo a los usuarios monitorear condiciones ambientales y detectar riesgos a distancia.

---

## ⚠️ Importante

Este sistema está diseñado con fines académicos y de investigación. No sustituye dispositivos certificados para la detección de gas en entornos industriales o comerciales.
