# 🛵 2WheelTracker-ESP32

Proyecto basado en el microcontrolador **ESP32** para crear un sistema de registro GPS con sensores, almacenando los datos en una **base de datos SQLite**.

---

## ⚙️ Pines utilizados

### 🌡️ Sensor DHT11
| Señal | Pin ESP32 |
|-------|-----------|
| VCC   | 3.3V      |
| DATA  | 36        |
| GND   | GND       |

---

### 📡 Módulo GPS (u-blox M8M)
| Señal | Pin ESP32     |
|-------|---------------|
| VCC   | 5V            |
| GND   | GND           |
| TXD   | RXD (42)      |
| RXD   | TXD (41)      |

---

### 💾 Módulo MicroSD (SPI)
| Señal | Pin ESP32 |
|-------|-----------|
| GND   | GND       |
| MISO  | 13        |
| CLK   | 12        |
| MOSI  | 11        |
| CS    | 10        |
| VCC   | 3.3V      |

---

### 🧭 Sensor ICM20948 v2 (Movimiento, IMU)
| Señal | Pin ESP32 |
|-------|-----------|
| VCC   | 3.3V      |
| GND   | GND       |
| SCL   | 39        |
| SDA   | 38        |
| ADO   | VCC       |

> ⚠️ Pines no conectados: `EDA`, `ECL`, `INT`, `NCS`, `FSYNC`

---

### 📺 Pantalla TFT SPI 2.4" (240x320)
| Señal      | Pin ESP32  |
|------------|------------|
| VCC        | 5V         |
| GND        | GND        |
| CS         | 18         |
| RESET      | 17         |
| DC         | 9          |
| SDI/MOSI   | 11         |
| SCK        | 12         |
| LED        | 5V         |
| SDO/MISO   | (opcional o pin 13) |

#### 🖱️ Touchscreen (XPT2046)
| Señal  | Pin ESP32 |
|--------|-----------|
| T_CLK  | 12        |
| T_CS   | 4         |
| T_DIN  | 11        |
| T_DO   | 13        |

---

## 📌 Notas

- Este proyecto está diseñado para **almacenar datos locales sin conexión a Internet**.
- Es posible conectar sensores adicionales gracias a la capacidad del ESP32.
- Ideal para seguimiento de rutas, registro de actividad o sistemas de alerta.

---

## 📷 Vista general del sistema

🚧 _Pendiente de incluir esquemas o imágenes del circuito_.

---

