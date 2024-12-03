#include <M5GFX.h>
#include <M5StickCPlus.h>
#include <M5_DLight.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>

// NTP and time settings
const char* ntpServer = "pool.ntp.org";
const char* timezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";  // Central European Time
long gmtOffset_sec = 3600;       // GMT+1
int daylightOffset_sec = 3600;   // Daylight Saving Time offset

// Define the display
M5GFX display;
M5Canvas canvas(&display);

// DLight sensor
M5_DLight sensor;
uint16_t lux;

// MQTT settings (Change these to your Mosquitto settings)
const char* mqtt_server = "192.168.x.x";  // Replace with your Mosquitto broker IP address or hostname
const int mqtt_port = 1883;  // Default Mosquitto MQTT port
const char* mqtt_topic = "m5stickc/ambientLux";  // Topic to publish the data

// Optional MQTT Authentication (remove if not needed)
const char* mqtt_user = "userName";  // Set if Mosquitto has authentication
const char* mqtt_password = "password";  // Set if Mosquitto has authentication

// WiFi settings
const char* ssid = "xxx";  // Replace with your WiFi SSID
const char* password = "xxx";  // Replace with your WiFi password

// MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// Info string
char info[40];
char batteryInfo[40];
char timeString[40];

// MQTT callback function (optional, for incoming messages)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Handle incoming MQTT messages (if needed)
}

void setup() {
    M5.begin();
    M5.Axp.begin();  // Initializes the AXP192 power management IC
    M5.Lcd.begin();  // Initialize Display
    M5.Lcd.setRotation(3);

    Serial.begin(115200);  // Start serial communication for debugging
    display.begin();
    canvas.setTextDatum(MC_DATUM);
    canvas.setColorDepth(1);
    canvas.setFont(&fonts::Orbitron_Light_24);
    canvas.setTextSize(1);
    canvas.setRotation(3);
    canvas.createSprite(display.width(), display.height());
    canvas.setPaletteColor(1, ORANGE);

    Serial.println("Sensor begin.....");
    sensor.begin();

    // Set the sensor mode
    sensor.setMode(CONTINUOUSLY_H_RESOLUTION_MODE);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");

    // Configure NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeInfo;
    while (!getLocalTime(&timeInfo)) {
        Serial.println("Waiting for time sync...");
        delay(1000);
    }
    Serial.println("Time synchronized!");

    // Setup MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqttCallback);

    if (client.connect("M5StickCPlusClient", mqtt_user, mqtt_password)) {
        Serial.println("MQTT connected with authentication");
    } else {
        Serial.println("MQTT connected without authentication");
    }
}

// MQTT reconnect function
void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("M5StickCPlusClient", mqtt_user, mqtt_password)) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();  // Keep the MQTT connection alive

    lux = sensor.getLUX();
    float batteryVoltage = M5.Axp.GetBatVoltage();  // Read battery voltage

    sprintf(info, "lux: %d", lux);
    sprintf(batteryInfo, "Battery: %.2fV", batteryVoltage);

    struct tm timeInfo;
    if (getLocalTime(&timeInfo)) {
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeInfo);
    } else {
        strcpy(timeString, "Time unavailable");
    }

    canvas.fillSprite(BLACK);
    canvas.drawString(info, 20, 30);
    canvas.drawString(batteryInfo, 20, 70);
    canvas.drawString(timeString, 20, 100);
    canvas.pushSprite(0, 0);

    Serial.println(info);
    Serial.println(batteryInfo);
    Serial.println(timeString);

    String luxString = String(lux);
    String batteryString = String(batteryVoltage, 2);
    client.publish(mqtt_topic, luxString.c_str());
    client.publish("m5stickc/batteryVoltage", batteryString.c_str());

    M5.update();  // Refresh M5 library state
    delay(1000);
}