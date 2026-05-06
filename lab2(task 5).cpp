#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- Конфігурація апаратних пінів ---
const int PIN_PHOTORESISTOR = 4;    // Аналоговий вхід для світла
const int PIN_1WIRE_BUS = 5;        // Цифровий вхід/вихід для DS18B20

// --- Створення об'єктів для роботи з протоколами ---
Adafruit_BMP280 myBarometer;                    // I2C працює на стандартних пінах (SDA=21, SCL=22)
OneWire oneWireProtocol(PIN_1WIRE_BUS);
DallasTemperature myDsSensors(&oneWireProtocol);

void setup() {
    Serial.begin(115200);
    Serial.println(">>> Ініціалізація комплексної системи моніторингу <<<");

    // 1. Налаштовуємо I2C барометр
    if (!myBarometer.begin(0x76)) {
        Serial.println("Помилка: BMP280 не виявлено. Перевірте контакти SDA/SCL.");
    }

    // 2. Налаштовуємо 1-Wire шину для температурного датчика
    myDsSensors.begin();

    // 3. Конфігуруємо пін фоторезистора як вхід (на всяк випадок)
    pinMode(PIN_PHOTORESISTOR, INPUT);
}

void loop() {
    Serial.println("\n--- Отримання нових даних ---");

    // БЛОК 1: Робота з протоколом 1-Wire (DS18B20)
    myDsSensors.requestTemperatures(); // Просимо виміряти
    float tempFromDS = myDsSensors.getTempCByIndex(0); // Зчитуємо перший (і єдиний) датчик
    Serial.print("[1-Wire] Температура DS18B20: ");
    Serial.print(tempFromDS);
    Serial.println(" °C");

    // БЛОК 2: Робота з протоколом I2C (BMP280)
    float tempFromBMP = myBarometer.readTemperature();
    float pressureFromBMP = myBarometer.readPressure() * 0.00750062; // Перевод у міліметри
    Serial.print("[I2C]    Температура BMP280:  ");
    Serial.print(tempFromBMP);
    Serial.print(" °C | Атм. тиск: ");
    Serial.print(pressureFromBMP);
    Serial.println(" мм рт. ст.");

    // БЛОК 3: Робота з АЦП (Фоторезистор)
    int lightADCValue = analogRead(PIN_PHOTORESISTOR);
    Serial.print("[АЦП]    Поточна освітленість (сирі дані): ");
    Serial.println(lightADCValue);

    Serial.println("-----------------------------");

    // Затримка перед наступним опитуванням всього комплексу
    delay(2000);
}

