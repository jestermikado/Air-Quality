⚙️ Technologies & Skills Demonstrated

Embedded Programming: ESP32 (Arduino Framework)
IoT Communication: MQTT protocol using HiveMQ broker
Real-Time Multitasking: FreeRTOS with task queues
Sensors & Actuators: DHT11 for temperature/humidity, analog air sensor, RGB LED status indication
Display Interface: SSD1306 OLED for live monitoring
Data Processing: Rolling average of sensor data for AQI smoothing

🧩 System Features

📶 WiFi Connectivity — ESP32 automatically connects and streams sensor data to the cloud
☁️ MQTT Publishing — Live temperature, humidity, and AQI data published to HiveMQ topic
🧠 FreeRTOS Task Separation —
temptask() → Reads DHT11 sensor
sensortask() → Reads analog air sensor
controltask() → Calculates average, updates display & LED, publishes MQTT data
🌈 RGB LED Indicator —
Green: Good Air
Orange: Moderate
Red: Poor Air
🖥️ OLED Display Output — Displays real-time temperature, humidity, and air status

🔌 Hardware Components

ESP32 - Dev Board	Main microcontroller
DHT11 Sensor -	Measures temperature & humidity
MQ135 / Analog Air Sensor	- Measures air quality (dust/gas concentration)
SSD1306 OLED -	Displays data locally
RGB LED -	Visual air quality indicator
Jumper Wires & Breadboard	Circuit connections

🧠 Working Principle

Sensors collect environmental data.
ESP32 tasks (under FreeRTOS) process readings concurrently.
Averaged data is visualized on the OLED and color-coded via RGB LED.
Data is published to HiveMQ MQTT broker for remote monitoring.
