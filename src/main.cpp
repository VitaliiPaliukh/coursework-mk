#include <arduino.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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


void setup()
{
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

        if (humidity < 80.0) {
            digitalWrite(RELAYPIN, HIGH);
        }
        else {
            digitalWrite(RELAYPIN,LOW);
        }

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
        display.print("Temperature: ");
        display.print(temperature);
        display.println(" C");

        display.print("Humidity:  ");
        display.print(humidity);
        display.println(" %");

        display.display();
    }
}