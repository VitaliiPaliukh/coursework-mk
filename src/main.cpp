#include <arduino.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define SSID "TP-LINK_4408"
#define PASSWORD "26470467"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 14
#define DHTTYPE DHT11
#define RELAYPIN 13

DHT dht(DHTPIN, DHTTYPE);

#define readInterval 2000
uint32_t previousReadMillis = 0;

void sendToBackend(float temperature, float humidity);
void wifiSetup();
void getCurrentCondition();
float getTargetHumidity();

void setup()
{
    wifiSetup();
    pinMode(RELAYPIN, OUTPUT);
    digitalWrite(RELAYPIN, HIGH);
    Serial.begin(9600);
    dht.begin();

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
        {
        Serial.println(F("SSD1306 initialization failed"));
        while (true);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("DHT11 Monitor"));
    display.display();
    delay(2000);
}

void wifiSetup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);

    uint32_t startAttemptTime = millis();
    uint32_t checkInterval = 500;
    uint32_t lastCheckTime = 0;

    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime < 15000)) {
        if (millis() - lastCheckTime >= checkInterval) {
            lastCheckTime = millis();
            Serial.println("Connecting to Wi-Fi...");
        }
        yield();
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("IP ESP8266: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("cant connect to wifi.");
    }
}

void loop()
{
    uint32_t currentMillis = millis();

    if (currentMillis - previousReadMillis >= readInterval)
        {
        previousReadMillis = currentMillis;

        float humidity = dht.readHumidity();
        float temperature = dht.readTemperature();


        if (isnan(humidity) || isnan(temperature)) {
            Serial.println("ERROR reading DHT11!");

            display.clearDisplay();
            display.setCursor(0, 0);
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.println("Sensor error!");
            display.display();

            delay(2000);
            return;
        }
        float targetHumidity = getTargetHumidity();
        if (humidity < targetHumidity) {
            digitalWrite(RELAYPIN, HIGH);
        }
        else {
            digitalWrite(RELAYPIN,LOW);
        }
        Serial.print("targetHumidity: ");
        Serial.print(targetHumidity);
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print(" %\t");

        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.println(" *C");

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("DHT11 Data:");
        display.print("targetHumidity: ");
        display.print(targetHumidity);
        display.print("Temperature: ");
        display.print(temperature);
        display.println(" C");

        display.print("Humidity:  ");
        display.print(humidity);
        display.println(" %");

        display.display();
        sendToBackend(temperature, humidity);
        getTargetHumidity();
    }
}

void sendToBackend(float temperature, float humidity) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        WiFiClient client;

        http.begin(client, "http://192.168.0.100:8000/api/v1/keypress/");
        http.addHeader("Content-Type", "application/json");

        String json = "{\"device_name\":\"ESP8266-DHT11\","
                      "\"temperature\":" + String(temperature, 1) +
                      ",\"humidity\":" + String(humidity, 1) + "}";

        int httpResponseCode = http.POST(json);

        if (httpResponseCode > 0) {
            Serial.println("Data sent! Code: " + String(httpResponseCode));
            Serial.println("Response: " + http.getString());
        } else {
            Serial.println("Failed to send data. Code: " + String(httpResponseCode));
        }

        http.end();
    } else {
        Serial.println("WiFi not connected.");
    }
}

float getTargetHumidity() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, "http://192.168.0.100:8000/api/v1/target_humidity");

        int httpCode = http.GET();

        if (httpCode == 200) {
            String payload = http.getString();
            Serial.println("Response:");
            Serial.println(payload);

            int index = payload.indexOf("target_humidity");
            if (index != -1) {
                int colon = payload.indexOf(":", index);
                int comma = payload.indexOf(",", colon);
                if (comma == -1) comma = payload.indexOf("}", colon);
                String value = payload.substring(colon + 1, comma);
                return value.toFloat();
            }
        } else {
            Serial.print("GET failed, error: ");
            Serial.println(httpCode);
        }

        http.end();
    } else {
        Serial.println("WiFi not connected");
    }

    return -1.0;
}

