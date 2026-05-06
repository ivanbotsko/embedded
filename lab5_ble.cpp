/*
 * Лабораторна робота №5
 * Студент: Боцко І.В., група ІК-31
 * Комплексний моніторинг датчиків із записом у SPIFFS та передачею по BLE
 */

#include <Arduino.h>              // Основна бібліотека Arduino
#include <OneWire.h>              // Бібліотека для протоколу 1-Wire
#include <DallasTemperature.h>    // Бібліотека для датчика температури DS18B20
#include <Wire.h>                 // Бібліотека для протоколу I2C
#include <Adafruit_BMP280.h>      // Бібліотека для барометра BMP280
#include "SPIFFS.h"               // Бібліотека для роботи з файловою системою у Flash-пам'яті
#include <BLEDevice.h>            // Головна бібліотека для роботи з Bluetooth Low Energy
#include <BLEServer.h>            // Бібліотека для створення BLE-сервера
#include <BLEUtils.h>             // Допоміжні утиліти BLE
#include <BLE2902.h>              // Дескриптор, необхідний для роботи сповіщень (Notify) в BLE

 // ********************* НАЛАШТУВАННЯ ПІНІВ *********************
#define DS18B20_PIN 32            // Пін, до якого підключений температурний датчик DS18B20
#define PHOTORESISTOR_PIN 34      // Пін, до якого підключений фоторезистор (АЦП)

// ********************* ІНІЦІАЛІЗАЦІЯ ОБ'ЄКТІВ ******************
OneWire oneWireBus(DS18B20_PIN);            // Створюємо об'єкт шини 1-Wire на вказаному піні
DallasTemperature myTempSensor(&oneWireBus);// Передаємо керування шиною бібліотеці датчика
Adafruit_BMP280 myBarometer;                // Створюємо об'єкт для I2C барометра (SDA=21, SCL=22 за замовчуванням)

// ********************* НАЛАШТУВАННЯ ТАЙМЕРА ********************
unsigned long lastReadTime = 0;             // Змінна для збереження часу останнього зчитування
const long readingInterval = 5000;          // Інтервал між зчитуваннями у мілісекундах (5 секунд)

// *********** BLE UUID (УНІКАЛЬНІ ІДЕНТИФІКАТОРИ) ***************
// Змінено базовий UUID для унікальності твого пристрою
#define MY_SERVICE_UUID "a1ea81f0-0e1b-d4a1-8840-63f88c8da1ea"
#define CHAR_TEMP_UUID  "a1ea81f0-0e1b-d4a1-8840-63f88c8da1eb"
#define CHAR_PRESS_UUID "a1ea81f0-0e1b-d4a1-8840-63f88c8da1ec"
#define CHAR_ALT_UUID   "a1ea81f0-0e1b-d4a1-8840-63f88c8da1ed"
#define CHAR_LUX_UUID   "a1ea81f0-0e1b-d4a1-8840-63f88c8da1ee"

// Вказівники на характеристики BLE, куди ми будемо записувати дані
BLECharacteristic* bleTemp;
BLECharacteristic* blePress;
BLECharacteristic* bleAlt;
BLECharacteristic* bleLux;

// *********** ФУНКЦІЯ ЗАПИСУ В ФАЙЛ (SPIFFS) ********************
void logDataToSPIFFS(float t, float p, float a, int l) {
    // Відкриваємо файл data.csv у режимі додавання (FILE_APPEND)
    File logFile = SPIFFS.open("/data.csv", FILE_APPEND);
    if (!logFile) { // Якщо файл не відкрився, виводимо помилку
        Serial.println("Помилка відкриття файлу data.csv!");
        return;     // Виходимо з функції
    }
    // Формуємо рядок формату CSV: час,темп,тиск,висота,світло
    String csvRow = String(millis()) + "," + String(t, 2) + "," + String(p, 2) + "," + String(a, 2) + "," + String(l) + "\n";
    logFile.print(csvRow); // Записуємо рядок у файл
    logFile.close();       // Обов'язково закриваємо файл для збереження
}

// ***************** ФУНКЦІЯ ЧИТАННЯ З ФАЙЛУ ***********************
// Ця функція шукає останній записаний рядок у файлі та розбиває його на змінні
bool fetchLatestRecord(float& t, float& p, float& a, int& l) {
    if (!SPIFFS.exists("/data.csv")) return false; // Якщо файлу немає, повертаємо false

    File logFile = SPIFFS.open("/data.csv", FILE_READ); // Відкриваємо на читання
    if (!logFile) return false;

    String finalLine;
    while (logFile.available()) { // Читаємо файл до самого кінця
        String currentLine = logFile.readStringUntil('\n'); // Читаємо рядок до знаку переносу
        if (currentLine.length() > 0) finalLine = currentLine; // Зберігаємо останній непорожній рядок
    }
    logFile.close();

    if (finalLine.length() == 0) return false;

    // Шукаємо позиції ком (розділювачів CSV)
    int pos1 = finalLine.indexOf(',');
    int pos2 = finalLine.indexOf(',', pos1 + 1);
    int pos3 = finalLine.indexOf(',', pos2 + 1);
    int pos4 = finalLine.indexOf(',', pos3 + 1);

    // Вирізаємо шматки тексту між комами і перетворюємо у числа (float або int)
    t = finalLine.substring(pos1 + 1, pos2).toFloat();
    p = finalLine.substring(pos2 + 1, pos3).toFloat();
    a = finalLine.substring(pos3 + 1, pos4).toFloat();
    l = finalLine.substring(pos4 + 1).toInt();

    return true; // Успішно прочитано
}

// ********************* ГОЛОВНЕ НАЛАШТУВАННЯ *********************
void setup() {
    Serial.begin(115200); // Запуск послідовного порту

    // 1. Запуск файлової системи SPIFFS (true = форматувати, якщо помилка)
    if (!SPIFFS.begin(true)) {
        Serial.println("Помилка монтування SPIFFS");
        return;
    }

    // 2. Ініціалізація датчика температури
    myTempSensor.begin();

    // 3. Ініціалізація барометра за адресою 0x76
    if (!myBarometer.begin(0x76)) {
        Serial.println("BMP280 не підключено!");
        while (1); // Зупиняємо програму, якщо датчик не знайдено
    }

    // 4. Створення файлу та запис заголовків (якщо файлу ще немає)
    if (!SPIFFS.exists("/data.csv")) {
        File logFile = SPIFFS.open("/data.csv", FILE_WRITE); // Відкриваємо для запису
        logFile.println("time_ms,temp_C,press_Pa,alt_m,lux"); // Пишемо шапку таблиці
        logFile.close();
    }

    // 5. Налаштування Bluetooth (BLE)
    BLEDevice::init("Ivan_BLE_Station");                // Ініціалізація BLE та задання імені пристрою
    BLEServer* myServer = BLEDevice::createServer();    // Створюємо BLE-сервер
    BLEService* myService = myServer->createService(MY_SERVICE_UUID); // Створюємо сервіс (папку для даних)

    // Створюємо характеристики (конкретні комірки для даних) і дозволяємо читання та сповіщення
    bleTemp = myService->createCharacteristic(CHAR_TEMP_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    bleTemp->addDescriptor(new BLE2902()); // Дескриптор для активації сповіщень на телефон

    blePress = myService->createCharacteristic(CHAR_PRESS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    blePress->addDescriptor(new BLE2902());

    bleAlt = myService->createCharacteristic(CHAR_ALT_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    bleAlt->addDescriptor(new BLE2902());

    bleLux = myService->createCharacteristic(CHAR_LUX_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    bleLux->addDescriptor(new BLE2902());

    myService->start(); // Запускаємо сервіс

    // Налаштовуємо "Рекламу" (Advertising), щоб телефон міг знайти плату
    BLEAdvertising* myAd = BLEDevice::getAdvertising();
    myAd->addServiceUUID(MY_SERVICE_UUID);
    myAd->setScanResponse(true);
    myAd->start();
    Serial.println("BLE сервер запущено. Очікування підключення...");
}

// ********************* ГОЛОВНИЙ ЦИКЛ *********************
void loop() {
    unsigned long currentTime = millis(); // Отримуємо поточний час роботи плати

    // Програмний таймер: виконуємо код лише якщо пройшло 5000 мс
    if (currentTime - lastReadTime >= readingInterval) {
        lastReadTime = currentTime; // Оновлюємо час останнього спрацювання

        // --- 1. Зчитування даних з датчиків ---
        myTempSensor.requestTemperatures(); // Просимо виміряти температуру
        float currentTemp = myTempSensor.getTempCByIndex(0); // Забираємо значення
        float currentPress = myBarometer.readPressure();     // Забираємо тиск (у Паскалях)
        float currentAlt = myBarometer.readAltitude(1013.25);// Висота над рівнем моря
        int currentLux = analogRead(PHOTORESISTOR_PIN);      // Зчитуємо АЦП з фоторезистора

        // --- 2. Вивід у монітор порту ---
        Serial.printf("T: %.2f C | P: %.2f Pa | Alt: %.2f m | Light: %d\n", currentTemp, currentPress, currentAlt, currentLux);

        // --- 3. Запис у пам'ять (файл) ---
        logDataToSPIFFS(currentTemp, currentPress, currentAlt, currentLux);

        // --- 4. Оновлення даних у Bluetooth (ВИПРАВЛЕНО) ---
        float savedTemp, savedPress, savedAlt;
        int savedLux;

        // Вичитуємо останні дані саме з файлу, як вимагає завдання
        if (fetchLatestRecord(savedTemp, savedPress, savedAlt, savedLux)) {
            // Записуємо вичитані з файлу дані у BLE характеристики
            bleTemp->setValue(String(savedTemp, 2).c_str());
            bleTemp->notify();

            blePress->setValue(String(savedPress, 2).c_str());
            blePress->notify();

            bleAlt->setValue(String(savedAlt, 2).c_str());
            bleAlt->notify();

            bleLux->setValue(String(savedLux).c_str());
            bleLux->notify();
        }
        else {
            Serial.println("Помилка читання з файлу для BLE!");
        }
}