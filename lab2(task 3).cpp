#include <SPI.h>
#include <Adafruit_BMP280.h>

// Для SPI нам потрібні індивідуальні піни вибору кристала (Chip Select)
// Лінії даних (SCK, MISO, MOSI) спільні для всіх
const int CHIP_SELECT_1 = 5;
const int CHIP_SELECT_2 = 17;

// Ініціалізуємо об'єкти, вказуючи конкретний CS пін для кожного
Adafruit_BMP280 sensorSPI_1(CHIP_SELECT_1);
Adafruit_BMP280 sensorSPI_2(CHIP_SELECT_2);

void setup() {
    Serial.begin(115200);
    Serial.println("Запуск опитування датчиків по апаратному SPI...");

    // Перевірка підключення першого модуля
    if (!sensorSPI_1.begin()) {
        Serial.println("Критична помилка: Датчик 1 (CS пін 5) не знайдено!");
    }

    // Перевірка підключення другого модуля
    if (!sensorSPI_2.begin()) {
        Serial.println("Критична помилка: Датчик 2 (CS пін 17) не знайдено!");
    }
}

void loop() {
    // Опитуємо перший датчик
    // Коли викликаються ці функції, мікроконтролер автоматично опускає рівень на піні CS_1 в LOW
    Serial.print("Сенсор 1 (SPI) -> T: ");
    Serial.print(sensorSPI_1.readTemperature());
    Serial.print(" °C, P: ");
    Serial.print(sensorSPI_1.readPressure() * 0.00750062);
    Serial.println(" мм рт. ст.");

    // Опитуємо другий датчик (рівень LOW подається на CS_2)
    Serial.print("Сенсор 2 (SPI) -> T: ");
    Serial.print(sensorSPI_2.readTemperature());
    Serial.print(" °C, P: ");
    Serial.print(sensorSPI_2.readPressure() * 0.00750062);
    Serial.println(" мм рт. ст.");

    Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~");
    delay(2000);
}
