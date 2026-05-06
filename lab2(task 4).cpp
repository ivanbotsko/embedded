#include <OneWire.h>
#include <DallasTemperature.h>

// Вказуємо єдиний пін, до якого підключена інформаційна лінія всіх датчиків
const int DATA_PIN_1WIRE = 4;

// Налаштовуємо базовий об'єкт 1-Wire
OneWire oneWireBus(DATA_PIN_1WIRE);

// Передаємо налаштовану шину в бібліотеку для роботи саме з датчиками температури
DallasTemperature tempSensors(&oneWireBus);

void setup() {
    Serial.begin(115200);

    // Стартуємо роботу на шині 1-Wire
    tempSensors.begin();

    // Рахуємо кількість пристроїв, які відгукнулися на шині
    int totalDevices = tempSensors.getDeviceCount();
    Serial.print("Система виявила температурних сенсорів: ");
    Serial.println(totalDevices);
}

void loop() {
    // Надсилаємо глобальну команду по шині: "всім датчикам розпочати вимірювання температури"
    tempSensors.requestTemperatures();

    // Проходимо циклом по всіх трьох датчиках (індекси від 0 до 2)
    for (int currentSensor = 0; currentSensor < 3; currentSensor++) {
        // Зчитуємо вже виміряну температуру за індексом датчика
        float currentTemp = tempSensors.getTempCByIndex(currentSensor);

        Serial.print("Сенсор №");
        Serial.print(currentSensor + 1); // Додаємо 1, щоб вивід був звичним (1, 2, 3), а не (0, 1, 2)
        Serial.print(" фіксує: ");
        Serial.print(currentTemp);
        Serial.println(" °C");
    }

    Serial.println("*************************");
    delay(1000);
}
