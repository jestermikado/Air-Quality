#include <WiFi.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define SENSOR 34
#define RED_PIN 25
#define GREEN_PIN 26
#define BLUE_PIN 27

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

DHT dht(DHTPIN, DHTTYPE);

const char* wifi = "WiFi_Name";
const char* password = "WiFi_Password";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espclient;
PubSubClient client(espclient);

float data_list[10];
float data_avg = 0;
float payload_temp = 0;
float payload_humid = 0;
int list_index = 0;
int count = 0;

QueueHandle_t dhtqueue;
QueueHandle_t sensorqueue;

typedef struct {
  float temperature;
  float humidity;
} dhtvalue;

typedef struct {
  float sensor_value;
} sensorvalue;

void setup_wifi();
void temptask(void* pv);
void sensortask(void* pv);
void controltask(void* pv);
void setLED(int r, int g, int b);
void showDisplay();
void publishData();
void reconnect();

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(BLUE_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(SENSOR, INPUT);
  Wire.begin();

  ledcAttachPin(RED_PIN, 0);
  ledcAttachPin(GREEN_PIN, 1);
  ledcAttachPin(BLUE_PIN, 2);
  ledcSetup(0, 5000, 8);
  ledcSetup(1, 5000, 8);
  ledcSetup(2, 5000, 8);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Hello.....");
  display.display();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.connect("ESP32_AQI_Client");

  dhtqueue = xQueueCreate(10, sizeof(dhtvalue));
  sensorqueue = xQueueCreate(5, sizeof(sensorvalue));

  xTaskCreatePinnedToCore(temptask, "temptask", 4096, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(sensortask, "sensortask", 4096, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(controltask, "controltask", 4096, NULL, 2, NULL, 1);
}

void setup_wifi() {
  WiFi.begin(wifi, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void temptask(void* pv) {
  dhtvalue dhtdata;
  for (;;) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println("inappropriate dht values");
    } else {
      dhtdata.temperature = t;
      dhtdata.humidity = h;
      xQueueSend(dhtqueue, &dhtdata, (TickType_t)100);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void sensortask(void* pv) {
  sensorvalue sensordata;
  for (;;) {
    int sensor_value = analogRead(SENSOR);
    float map_data = map(sensor_value, 0, 4095, 0, 500);
    sensordata.sensor_value = map_data;
    xQueueSend(sensorqueue, &sensordata, (TickType_t)100);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void controltask(void* pv) {
  dhtvalue controldht;
  sensorvalue controlsensor;

  for (;;) {
    if (xQueueReceive(sensorqueue, &controlsensor, (TickType_t)100) == pdTRUE) {
      data_list[list_index] = controlsensor.sensor_value;
      list_index = (list_index + 1) % 10;

      if (count < 10) {
        count += 1;
        data_avg = controlsensor.sensor_value;
      } else {
        float sum = 0;
        for (int i = 0; i < 10; i++) sum += data_list[i];
        data_avg = sum / 10.0;
      }

      if (data_avg < 100) {
        setLED(0, 255, 0);      
      } else if (data_avg < 200) {
        setLED(255, 165, 0);    
      } else {
        setLED(255, 0, 0);      
      }
    }

    if (xQueueReceive(dhtqueue, &controldht, (TickType_t)100) == pdTRUE) {
      payload_temp = controldht.temperature;
      payload_humid = controldht.humidity;
    }

    showDisplay();
    publishData();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setLED(int r, int g, int b) {
  ledcWrite(0, r);
  ledcWrite(1, g);
  ledcWrite(2, b);
}

void showDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,10);
  display.printf("Temp: %.1f C\nHum: %.1f %%\nAvg data: %.1f\n", payload_temp, payload_humid, data_avg);

  if (data_avg < 100) display.print("Status: Good");
  else if (data_avg < 200) display.print("Status: Moderate");
  else display.print("Status: Poor");

  display.display();
}

void publishData() {
  char payload[100];
  snprintf(payload, sizeof(payload), "{\"AVG Data\":%.2f,\"Temp\":%.2f,\"Hum\":%.2f}", data_avg, payload_temp, payload_humid);
  client.publish("Air_Quality", payload);
  Serial.println(payload);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32_AQI_Client")) {
      Serial.println("connected");
      client.subscribe("esp32/control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying...");
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();
}
