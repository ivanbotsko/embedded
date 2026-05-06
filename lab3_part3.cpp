/*
 * Лабораторна робота №3 (Підсумкове Завдання 3)
 * Виконав: Боцко І.В., група ІК-31
 * Завдання: Зчитування I2C барометра по таймеру + вимірювання тривалості затискання кнопки
 */

#include <Arduino.h>             // Базова бібліотека
#include <Wire.h>                // Бібліотека для протоколу I2C
#include <Adafruit_Sensor.h>     // Базова бібліотека датчиків від Adafruit
#include <Adafruit_BMP280.h>     // Специфічна бібліотека для барометра BMP280

 // ****************************  НАЛАШТУВАННЯ ПІНІВ  *****************************************
const int PIN_BTN_MEASURE = 18;  // Кнопка, яка фіксує тривалість натискання
const int PIN_LED_TIMER = 16;  // Діод 1: Мигає, коли зчитуємо дані з барометра
const int PIN_LED_BUTTON = 17;  // Діод 2: Світиться стільки ж часу, скільки тримали кнопку

// *****************************   ОБ'ЄКТИ ДЛЯ ДАТЧИКІВ ***************************************
Adafruit_BMP280 sensorBarometer; // Створюємо об'єкт барометра для шини I2C

// ***************************** ТАЙМЕР ТА СИНХРОНІЗАЦІЯ **************************************
hw_timer_t* hwTimerBMP = NULL;   // Вказівник на наш апаратний таймер
portMUX_TYPE syncMutex = portMUX_INITIALIZER_UNLOCKED; // М'ютекс для захисту змінних у багатоядерній системі

// ***************************** ЗМІННІ-ПРАПОРЦІ ДЛЯ ПЕРЕРИВАНЬ *******************************
// Volatile каже процесору: "Обережно, ця змінна може змінитися в будь-яку мілісекунду!"
volatile bool isTimerTriggered = false;     // Прапорець для зчитування барометра
volatile uint32_t timeBtnPressed = 0;       // Зберігає мілісекунду, коли кнопку ЗАТИСНУЛИ
volatile uint32_t finalPressDuration = 0;   // Тут зберігається фінальна різниця часу (час утримання)
volatile bool isButtonReleased = false;     // Прапорець: кнопку ВІДПУСТИЛИ, можна світити діодом

// *****************************  ОБРОБНИКИ ПЕРЕРИВАНЬ (ISR) **********************************

// ISR Таймера: спрацьовує кожні 3 секунди
void IRAM_ATTR isrTimerBMP() {
    portENTER_CRITICAL_ISR(&syncMutex); // Блокуємо доступ для інших процесів
    isTimerTriggered = true;            // Кажемо головному циклу, що час читати барометр
    portEXIT_CRITICAL_ISR(&syncMutex);
}

// ISR Кнопки: спрацьовує на БУДЬ-ЯКУ зміну стану піна (і на натискання, і на відпускання)
void IRAM_ATTR isrButtonLogic() {
    uint32_t currentMillis = millis(); // Фіксуємо поточний час

    // digitalRead повертає 0 (LOW), якщо кнопку затиснуто (бо в нас PULLUP)
    bool isPressedNow = !digitalRead(PIN_BTN_MEASURE);

    portENTER_CRITICAL_ISR(&syncMutex);
    if (isPressedNow) {
        // Якщо кнопку щойно НАТИСНУЛИ - зберігаємо стартовий час
        timeBtnPressed = currentMillis;
    }
    else if (timeBtnPressed > 0) {
        // Якщо кнопку ВІДПУСТИЛИ (і до цього вона була натиснута)
        finalPressDuration = currentMillis - timeBtnPressed; // Розраховуємо тривалість: Час_Відпускання - Час_Натискання
        isButtonReleased = true;  // Піднімаємо прапорець для головного циклу
        timeBtnPressed = 0;       // Обнуляємо стартовий час для наступних кліків
    }
    portEXIT_CRITICAL_ISR(&syncMutex);
}

// ***************************** БЛОК НАЛАШТУВАННЯ *******************************************
void setup() {
    Serial.begin(115200);

    // Налаштовуємо піни
    pinMode(PIN_BTN_MEASURE, INPUT_PULLUP); // Кнопка підтягнута до 3.3В
    pinMode(PIN_LED_TIMER, OUTPUT);
    pinMode(PIN_LED_BUTTON, OUTPUT);

    // Ініціалізація барометра I2C (стандартна адреса 0x76)
    if (!sensorBarometer.begin(0x76)) {
        Serial.println("[ПОМИЛКА] Барометр BMP280 не виявлено! Перевірте підключення.");
    }

    // Налаштування таймера: 1 МГц (1 мкс), тривалість 3 секунди (3 000 000 мкс)
    hwTimerBMP = timerBegin(1000000);
    timerAttachInterrupt(hwTimerBMP, &isrTimerTimerBMP); // ПОМИЛКА ДРУКУ В ТВОЄМУ КОДІ ВИПРАВЛЕНА: &isrTimerBMP
    timerAlarm(hwTimerBMP, 3000000, true, 0);

    // Налаштування переривання кнопки. Режим CHANGE відслідковує і спад, і фронт сигналу
    attachInterrupt(PIN_BTN_MEASURE, isrButtonLogic, CHANGE);

    Serial.println("[ІК-31 Боцко] Система готова. Таймер 3с. Чекаю затискання кнопки...");
}

// ********************************** ГОЛОВНИЙ ЦИКЛ *****************************************
void loop() {

    // --- ЧАСТИНА 1: ЗЧИТУВАННЯ БАРОМЕТРА ПО ТАЙМЕРУ ---
    if (isTimerTriggered) {
        // Безпечно опускаємо прапорець
        portENTER_CRITICAL(&syncMutex);
        isTimerTriggered = false;
        portEXIT_CRITICAL(&syncMutex);

        // Звертатися до I2C (повільний протокол) всередині ISR суворо заборонено!
        // Тому ми вичитуємо дані тут, у безпечному головному циклі.
        float currentTemp = sensorBarometer.readTemperature();
        float currentPres = sensorBarometer.readPressure() / 100.0F; // Переводимо Паскалі у гектопаскалі (hPa)

        Serial.printf("[ТАЙМЕР] Температура: %.2f °C | Тиск: %.2f hPa\n", currentTemp, currentPres);

        // Блимаємо діодом, сигналізуючи про успішну вичитку
        digitalWrite(PIN_LED_TIMER, HIGH);
        delay(100);
        digitalWrite(PIN_LED_TIMER, LOW);
    }

    // --- ЧАСТИНА 2: ОБРОБКА ТРИВАЛОСТІ ЗАТИСКАННЯ КНОПКИ ---
    if (isButtonReleased) {
        // Безпечно забираємо розраховану тривалість і опускаємо прапорець
        portENTER_CRITICAL(&syncMutex);
        uint32_t holdDuration = finalPressDuration;
        isButtonReleased = false;
        portEXIT_CRITICAL(&syncMutex);

        Serial.printf("[КНОПКА] Була затиснута протягом: %u мілісекунд\n", holdDuration);

        // Засвічуємо другий діод рівно на той самий час, що і тривалість затискання[cite: 1]
        digitalWrite(PIN_LED_BUTTON, HIGH);
        delay(holdDuration);
        digitalWrite(PIN_LED_BUTTON, LOW);
    }
}