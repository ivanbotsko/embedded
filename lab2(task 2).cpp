#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// Створюємо два окремих об'єкти для кожного барометра
Adafruit_BMP280 barometerA;
Adafruit_BMP280 barometerB;

void setup() {
    Serial.begin(115200);
    Serial.println("Ініціалізація шини I2C та пошук пристроїв...");

    // I2C дозволяє підключати кілька пристроїв на 2 проводи (SDA, SCL).
    // Головне правило - вони повинні мати різні адреси.

    // Запускаємо перший барометр за адресою 0x76 (стандартна для більшості модулів)
    if (!barometerA.begin(0x76)) {
        Serial.println("Помилка: Барометр А (0x76) не відповідає.");
    }

    // Запускаємо другий барометр. Його адреса змінена на 0x77 апаратно (через пін SDO)
    if (!barometerB.begin(0x77)) {
        Serial.println("Помилка: Барометр Б (0x77) не відповідає.");
    }
}

void loop() {
    // Зчитуємо та форматуємо дані з першого пристрою
    Serial.print("Барометр [0x76] -> Темп: ");
    Serial.print(barometerA.readTemperature());
    Serial.print(" °C | Тиск: ");
    // Переводимо Паскалі у міліметри ртутного стовпа (коефіцієнт 0.0075)
    Serial.print(barometerA.readPressure() * 0.00750062);
    Serial.println(" мм рт. ст.");

    // Зчитуємо дані з другого пристрою
    Serial.print("Барометр [0x77] -> Темп: ");
    Serial.print(barometerB.readTemperature());
    Serial.print(" °C | Тиск: ");
    Serial.print(barometerB.readPressure() * 0.00750062);
    Serial.println(" мм рт. ст.");

    Serial.println("======================================");
    delay(2000); // Робимо паузу у 2 секунди між вимірами
}
