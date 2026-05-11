/*
 * FarmGuard-AIoT Firmware
 * Smart Irrigation & Microclimate Monitoring System
 * Target: ESP32 with FreeRTOS
 * Research Methodology in Computing - BCT 2315 / BCT 2402
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FreeRTOS.h>

// ============== CONFIGURATION ==============
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_SERVER = "your-mqtt-broker.com";
const int MQTT_PORT = 1883;
const char* DEVICE_ID = "FARMGUARD-001";
const char* LOCATION = "Plot-A-North";

// Sampling intervals (milliseconds)
const int SENSOR_READ_INTERVAL = 5000;   // 5 seconds
const int MQTT_PUBLISH_INTERVAL = 30000; // 30 seconds
const int DEEP_SLEEP_DURATION = 300000;  // 5 minutes (power saving)

// ============== PIN DEFINITIONS ==============
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define SOIL_MOISTURE_PIN 34
#define SOIL_PH_PIN 35
#define LIGHT_SENSOR_PIN 32
#define RAIN_SENSOR_PIN 33
#define WIND_SPEED_PIN 25
#define RELAY_PUMP_PIN 26
#define LED_STATUS_PIN 2

// ============== SENSOR OBJECTS ==============
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ============== DATA STRUCTURES ==============
struct SensorData {
    float temperature;
    float humidity;
    float soilMoisture;
    float soilPH;
    float lightIntensity;
    float rainfall;
    float windSpeed;
    unsigned long timestamp;
    bool pumpActive;
    float batteryVoltage;
    int rssi;
};

SensorData currentData;
SemaphoreHandle_t dataMutex;

// ============== FUNCTION PROTOTYPES ==============
void setupWiFi();
void setupMQTT();
void reconnectMQTT();
void readSensors(void* parameter);
void publishData(void* parameter);
void controlIrrigation(void* parameter);
float readSoilMoisture();
float readSoilPH();
float readLightIntensity();
float readRainfall();
float readWindSpeed();
float readBatteryVoltage();
void enterDeepSleep();

// ============== SETUP ==============
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("  FarmGuard-AIoT Firmware v1.0");
    Serial.println("  Smart Irrigation & Microclimate");
    Serial.println("  Research Methodology in Computing");
    Serial.println("========================================\n");

    // Initialize pins
    pinMode(LED_STATUS_PIN, OUTPUT);
    pinMode(RELAY_PUMP_PIN, OUTPUT);
    digitalWrite(RELAY_PUMP_PIN, LOW);

    // Initialize sensors
    dht.begin();

    // Create mutex for thread-safe data access
    dataMutex = xSemaphoreCreateMutex();

    // Setup connectivity
    setupWiFi();
    setupMQTT();

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(
        readSensors,
        "SensorTask",
        4096,
        NULL,
        1,
        NULL,
        1  // Core 1
    );

    xTaskCreatePinnedToCore(
        publishData,
        "MQTTPublishTask",
        4096,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        controlIrrigation,
        "IrrigationTask",
        2048,
        NULL,
        2,  // Higher priority
        NULL,
        1
    );

    Serial.println("[SETUP] All tasks created successfully");
    Serial.println("[SETUP] System operational\n");
}

void loop() {
    // Main loop handles MQTT connection maintenance
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.loop();

    // Blink status LED to indicate alive
    digitalWrite(LED_STATUS_PIN, HIGH);
    delay(100);
    digitalWrite(LED_STATUS_PIN, LOW);
    delay(900);
}

// ============== WIFI SETUP ==============
void setupWiFi() {
    Serial.print("[WIFI] Connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Connected successfully");
        Serial.print("[WIFI] IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("[WIFI] RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    } else {
        Serial.println("\n[WIFI] Connection failed! Entering deep sleep...");
        enterDeepSleep();
    }
}

// ============== MQTT SETUP ==============
void setupMQTT() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    Serial.print("[MQTT] Broker: ");
    Serial.print(MQTT_SERVER);
    Serial.print(":");
    Serial.println(MQTT_PORT);
}

void reconnectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("[MQTT] Attempting connection...");

        String clientId = "FarmGuard-" + String(DEVICE_ID);

        if (mqttClient.connect(clientId.c_str())) {
            Serial.println("connected");

            // Subscribe to control topics
            String cmdTopic = String("farmguard/") + DEVICE_ID + "/command";
            mqttClient.subscribe(cmdTopic.c_str());
            Serial.print("[MQTT] Subscribed to: ");
            Serial.println(cmdTopic);

            // Publish birth certificate
            String statusTopic = String("farmguard/") + DEVICE_ID + "/status";
            mqttClient.publish(statusTopic.c_str(), "online", true);
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" retry in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("[MQTT] Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");

    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);

    // Parse command
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (!error) {
        const char* command = doc["command"];

        if (strcmp(command, "irrigate") == 0) {
            int duration = doc["duration"] | 10;
            Serial.print("[CMD] Starting irrigation for ");
            Serial.print(duration);
            Serial.println(" minutes");

            digitalWrite(RELAY_PUMP_PIN, HIGH);
            currentData.pumpActive = true;

            // Schedule pump off
            vTaskDelay(duration * 60000 / portTICK_PERIOD_MS);

            digitalWrite(RELAY_PUMP_PIN, LOW);
            currentData.pumpActive = false;

            Serial.println("[CMD] Irrigation completed");
        }

        if (strcmp(command, "status") == 0) {
            // Trigger immediate data publish
            // (handled by publish task)
        }
    }
}

// ============== SENSOR READING TASK ==============
void readSensors(void* parameter) {
    Serial.println("[TASK] Sensor reading task started");

    for (;;) {
        // Take mutex
        if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
            // Read DHT22
            currentData.temperature = dht.readTemperature();
            currentData.humidity = dht.readHumidity();

            // Read other sensors
            currentData.soilMoisture = readSoilMoisture();
            currentData.soilPH = readSoilPH();
            currentData.lightIntensity = readLightIntensity();
            currentData.rainfall = readRainfall();
            currentData.windSpeed = readWindSpeed();

            // System metrics
            currentData.timestamp = millis();
            currentData.batteryVoltage = readBatteryVoltage();
            currentData.rssi = WiFi.RSSI();

            xSemaphoreGive(dataMutex);
        }

        // Print to serial for debugging
        Serial.println("\n[SENSOR] === Reading Cycle ===");
        Serial.print("  Temperature: "); Serial.print(currentData.temperature); Serial.println(" C");
        Serial.print("  Humidity: "); Serial.print(currentData.humidity); Serial.println(" %");
        Serial.print("  Soil Moisture: "); Serial.print(currentData.soilMoisture); Serial.println(" %");
        Serial.print("  Soil pH: "); Serial.print(currentData.soilPH); Serial.println(" pH");
        Serial.print("  Light: "); Serial.print(currentData.lightIntensity); Serial.println(" lux");
        Serial.print("  Rainfall: "); Serial.print(currentData.rainfall); Serial.println(" mm");
        Serial.print("  Wind: "); Serial.print(currentData.windSpeed); Serial.println(" m/s");
        Serial.print("  Battery: "); Serial.print(currentData.batteryVoltage); Serial.println(" V");
        Serial.print("  RSSI: "); Serial.print(currentData.rssi); Serial.println(" dBm");

        vTaskDelay(SENSOR_READ_INTERVAL / portTICK_PERIOD_MS);
    }
}

// ============== MQTT PUBLISH TASK ==============
void publishData(void* parameter) {
    Serial.println("[TASK] MQTT publish task started");

    for (;;) {
        if (mqttClient.connected()) {
            // Take mutex
            if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
                // Create JSON payload
                StaticJsonDocument<512> doc;

                doc["device_id"] = DEVICE_ID;
                doc["location"] = LOCATION;
                doc["timestamp"] = currentData.timestamp;
                doc["temperature"] = currentData.temperature;
                doc["humidity"] = currentData.humidity;
                doc["soil_moisture"] = currentData.soilMoisture;
                doc["soil_ph"] = currentData.soilPH;
                doc["light_intensity"] = currentData.lightIntensity;
                doc["rainfall_mm"] = currentData.rainfall;
                doc["wind_speed"] = currentData.windSpeed;
                doc["pump_active"] = currentData.pumpActive;
                doc["battery_voltage"] = currentData.batteryVoltage;
                doc["rssi"] = currentData.rssi;

                char payload[512];
                serializeJson(doc, payload);

                xSemaphoreGive(dataMutex);

                // Publish to MQTT
                String topic = String("farmguard/") + DEVICE_ID + "/data";
                mqttClient.publish(topic.c_str(), payload);

                Serial.print("[MQTT] Published to ");
                Serial.println(topic);
                Serial.print("[MQTT] Payload: ");
                Serial.println(payload);
            }
        }

        vTaskDelay(MQTT_PUBLISH_INTERVAL / portTICK_PERIOD_MS);
    }
}

// ============== IRRIGATION CONTROL TASK ==============
void controlIrrigation(void* parameter) {
    Serial.println("[TASK] Irrigation control task started");

    for (;;) {
        // Automatic irrigation logic
        if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
            // Trigger irrigation if soil moisture is critically low
            if (currentData.soilMoisture < 30.0 && !currentData.pumpActive) {
                Serial.println("[AUTO] Low soil moisture detected! Starting irrigation...");

                digitalWrite(RELAY_PUMP_PIN, HIGH);
                currentData.pumpActive = true;

                // Publish irrigation event
                StaticJsonDocument<256> doc;
                doc["event"] = "auto_irrigation_triggered";
                doc["soil_moisture"] = currentData.soilMoisture;
                doc["timestamp"] = currentData.timestamp;

                char payload[256];
                serializeJson(doc, payload);

                String topic = String("farmguard/") + DEVICE_ID + "/events";
                mqttClient.publish(topic.c_str(), payload);

                // Run pump for 10 minutes
                xSemaphoreGive(dataMutex);
                vTaskDelay(600000 / portTICK_PERIOD_MS); // 10 minutes

                if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
                    digitalWrite(RELAY_PUMP_PIN, LOW);
                    currentData.pumpActive = false;
                    Serial.println("[AUTO] Irrigation cycle completed");
                }
            }

            xSemaphoreGive(dataMutex);
        }

        vTaskDelay(60000 / portTICK_PERIOD_MS); // Check every minute
    }
}

// ============== SENSOR READING FUNCTIONS ==============
float readSoilMoisture() {
    int rawValue = analogRead(SOIL_MOISTURE_PIN);
    // Convert to percentage (calibrate for your sensor)
    float percentage = map(rawValue, 0, 4095, 100, 0);
    return constrain(percentage, 0, 100);
}

float readSoilPH() {
    int rawValue = analogRead(SOIL_PH_PIN);
    // Convert to pH (calibrate with pH 4.0 and 7.0 solutions)
    float voltage = rawValue * (3.3 / 4095.0);
    float phValue = 3.5 * voltage; // Simplified conversion
    return constrain(phValue, 0, 14);
}

float readLightIntensity() {
    int rawValue = analogRead(LIGHT_SENSOR_PIN);
    // Convert to lux (depends on photoresistor characteristics)
    float lux = rawValue * (1000.0 / 4095.0);
    return lux;
}

float readRainfall() {
    int rawValue = analogRead(RAIN_SENSOR_PIN);
    // Rain detection threshold
    if (rawValue < 1000) {
        return 0.5; // Simulated rainfall amount
    }
    return 0.0;
}

float readWindSpeed() {
    int rawValue = analogRead(WIND_SPEED_PIN);
    // Convert to m/s (calibrate with anemometer)
    float windSpeed = rawValue * (30.0 / 4095.0);
    return windSpeed;
}

float readBatteryVoltage() {
    int rawValue = analogRead(35); // Battery monitoring pin
    float voltage = rawValue * (3.3 / 4095.0) * 2; // Voltage divider
    return voltage;
}

// ============== POWER MANAGEMENT ==============
void enterDeepSleep() {
    Serial.println("[POWER] Entering deep sleep mode");
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION * 1000);
    esp_deep_sleep_start();
}
