/*
 * Лабораторна робота №4
 * Дослідження використання алгоритмів на мікроконтролері 
 * Виконав: Боцко І.В., група ІК-31
 * Завдання 1: Дослідження алгоритмів сортування та бінарного дерева у вбудованих системах
 */

#include <Arduino.h>  // Основна бібліотека для роботи з середовищем Arduino/ESP
#include "soc/rtc.h"  // Бібліотека для керування апаратними функціями (напр. частотою процесора)

 // ************************************* СТРУКТУРИ ДАНИХ ***************************************

 // Створюємо шаблонну структуру вузла дерева (дозволяє працювати з будь-яким типом даних)
template <typename DataType>
struct BstNode {
    DataType nodeValue;         // Змінна для зберігання безпосереднього значення
    BstNode* leftChild;         // Покажчик на ліву гілку (елементи, що менші за поточний)
    BstNode* rightChild;        // Покажчик на праву гілку (елементи, що більші або рівні)

    // Конструктор: ініціалізує новий вузол заданим значенням
    BstNode(DataType val) {
        nodeValue = val;        // Записуємо передане число/символ
        leftChild = nullptr;    // На початку лівого нащадка немає
        rightChild = nullptr;   // На початку правого нащадка немає
    }
};

// ************************************* АЛГОРИТМИ ДЛЯ ДЕРЕВА ***********************************

// Функція для додавання нового елемента у бінарне дерево
template <typename DataType>
BstNode<DataType>* addNodeToTree(BstNode<DataType>* currentNode, DataType newValue) {
    // Якщо знайшли вільне (порожнє) місце — створюємо тут новий вузол
    if (currentNode == nullptr) {
        return new BstNode<DataType>(newValue);
    }

    // Якщо нове значення менше поточного — спускаємось у ліву гілку
    if (newValue < currentNode->nodeValue) {
        currentNode->leftChild = addNodeToTree(currentNode->leftChild, newValue);
    }
    // Якщо більше або дорівнює — спускаємось у праву гілку
    else {
        currentNode->rightChild = addNodeToTree(currentNode->rightChild, newValue);
    }

    return currentNode; // Повертаємо поточний вузол, щоб зберегти структуру
}

// ************************************* АЛГОРИТМИ СОРТУВАННЯ ***********************************

// Допоміжна функція для заміни місцями двох елементів масиву (використовуємо посилання)
template <typename DataType>
void exchangeValues(DataType& firstItem, DataType& secondItem) {
    DataType temp = firstItem;  // Зберігаємо перше значення у тимчасову змінну
    firstItem = secondItem;     // Записуємо друге значення на місце першого
    secondItem = temp;          // Записуємо тимчасове значення на місце другого
}

// Функція розділення масиву (обирає опорний елемент та сортує відносно нього)
template <typename DataType>
int splitArray(DataType targetArray[], int startIdx, int endIdx) {
    DataType pivotElement = targetArray[endIdx]; // За опорний беремо останній елемент
    int smallerElementIdx = (startIdx - 1);      // Індекс найменшого елемента

    // Проходимо по масиву та переносимо менші за опорний елемент вліво
    for (int currentIdx = startIdx; currentIdx <= endIdx - 1; currentIdx++) {
        if (targetArray[currentIdx] < pivotElement) {
            smallerElementIdx++;                 // Зрушуємо межу менших елементів
            exchangeValues(targetArray[smallerElementIdx], targetArray[currentIdx]);
        }
    }
    // Ставимо опорний елемент на його правильне місце
    exchangeValues(targetArray[smallerElementIdx + 1], targetArray[endIdx]);
    return (smallerElementIdx + 1); // Повертаємо індекс опорного елемента
}

// Основна рекурсивна функція швидкого сортування (QuickSort) [cite: 9]
template <typename DataType>
void fastSort(DataType targetArray[], int startIdx, int endIdx) {
    if (startIdx < endIdx) {
        // Знаходимо позицію розділення
        int pivotPosition = splitArray(targetArray, startIdx, endIdx);
        // Рекурсивно сортуємо ліву та праву частини
        fastSort(targetArray, startIdx, pivotPosition - 1);
        fastSort(targetArray, pivotPosition + 1, endIdx);
    }
}

// ************************************* НАЛАШТУВАННЯ СИСТЕМИ ***********************************

void setup() {
    Serial.begin(115200); // Ініціалізація послідовного порту для виводу даних

    // Встановлюємо частоту роботи процесора (можна змінювати згідно завдання [cite: 12])
    setCpuFrequencyMhz(240);
    delay(10); // Невелика затримка для стабілізації
    Serial.println("\n[ІК-31] Ініціалізація алгоритмів... Старт!");

    // Задаємо розміри масивів згідно умов (50, 100, 500, 1000) [cite: 6]
    const int SIZE_INT = 1000;
    const int SIZE_DOUBLE = 100;
    const int SIZE_CHAR = 50;
    const int SIZE_FLOAT = 500;

    // Створюємо статичні масиви різних типів даних [cite: 7]
    static int dataInt[SIZE_INT];
    static double dataDouble[SIZE_DOUBLE];
    static char dataChar[SIZE_CHAR];
    static float dataFloat[SIZE_FLOAT];

    // Змінні для профілювання часу та пам'яті
    unsigned long timerStart, timerEnd, durationUs;
    uint32_t heapStart, heapEnd, memoryConsumed;

    // Масиви для збереження фінальних результатів (для звіту)
    unsigned long timeStats[8]; // Індекси 0-3: сортування, 4-7: дерева
    uint32_t memStats[8];       // Індекси 0-3: сортування, 4-7: дерева

    // --- ЕТАП 1: ГЕНЕРАЦІЯ ВИПАДКОВИХ ДАНИХ ---
    for (int i = 0; i < SIZE_INT; i++) dataInt[i] = esp_random() % 1000;
    for (int i = 0; i < SIZE_DOUBLE; i++) dataDouble[i] = (double)(esp_random() % 100);
    for (int i = 0; i < SIZE_CHAR; i++) dataChar[i] = 'a' + (esp_random() % 26);
    for (int i = 0; i < SIZE_FLOAT; i++) dataFloat[i] = (esp_random() % 1000) / 100.0f;

    // --- ЕТАП 2: ШВИДКЕ СОРТУВАННЯ ТА ВИМІРЮВАННЯ ---

    // 1. Сортування INT
    heapStart = ESP.getFreeHeap();          // Записуємо кількість вільної пам'яті до старту
    timerStart = micros();                  // Запускаємо мікросекундний таймер
    fastSort(dataInt, 0, SIZE_INT - 1);     // Викликаємо алгоритм
    timerEnd = micros();                    // Зупиняємо таймер
    durationUs = timerEnd - timerStart;     // Вираховуємо витрачений час
    heapEnd = ESP.getFreeHeap();            // Перевіряємо вільну пам'ять після завершення
    memoryConsumed = heapStart - heapEnd;   // Визначаємо скільки пам'яті пішло на алгоритм

    // Вивід результатів для INT
    Serial.printf("Сортування INT: %lu мкс | Витрачено пам'яті: %u байт\n", durationUs, memoryConsumed);
    timeStats[0] = durationUs; memStats[0] = memoryConsumed; // Зберігаємо для фінальної таблиці

    // 2. Сортування DOUBLE
    heapStart = ESP.getFreeHeap();
    timerStart = micros();
    fastSort(dataDouble, 0, SIZE_DOUBLE - 1);
    timerEnd = micros();
    durationUs = timerEnd - timerStart;
    heapEnd = ESP.getFreeHeap();
    memoryConsumed = heapStart - heapEnd;

    Serial.printf("Сортування DOUBLE: %lu мкс | Витрачено пам'яті: %u байт\n", durationUs, memoryConsumed);
    timeStats[1] = durationUs; memStats[1] = memoryConsumed;

    // 3. Сортування CHAR
    heapStart = ESP.getFreeHeap();
    timerStart = micros();
    fastSort(dataChar, 0, SIZE_CHAR - 1);
    timerEnd = micros();
    durationUs = timerEnd - timerStart;
    heapEnd = ESP.getFreeHeap();
    memoryConsumed = heapStart - heapEnd;

    Serial.printf("Сортування CHAR: %lu мкс | Витрачено пам'яті: %u байт\n", durationUs, memoryConsumed);
    timeStats[2] = durationUs; memStats[2] = memoryConsumed;

    // 4. Сортування FLOAT
    heapStart = ESP.getFreeHeap();
    timerStart = micros();
    fastSort(dataFloat, 0, SIZE_FLOAT - 1);
    timerEnd = micros();
    durationUs = timerEnd - timerStart;
    heapEnd = ESP.getFreeHeap();
    memoryConsumed = heapStart - heapEnd;

    Serial.printf("Сортування FLOAT: %lu мкс | Витрачено пам'яті: %u байт\n", durationUs, memoryConsumed);
    timeStats[3] = durationUs; memStats[3] = memoryConsumed;

    // --- ЕТАП 3: ВИВІД ВІДСОРТОВАНИХ ДАНИХ (фрагментарно, щоб не засмічувати консоль) ---
    Serial.print("\nРезультат сортування Int (перші 10): ");
    for (int i = 0; i < 10; i++) Serial.printf("%d, ", dataInt[i]);

    Serial.print("\nРезультат сортування Char (всі): ");
    for (int i = 0; i < SIZE_CHAR; i++) Serial.printf("%c", dataChar[i]);
    Serial.println("\n");

    // --- ЕТАП 4: ПОБУДОВА БІНАРНИХ ДЕРЕВ [cite: 11] ---
    Serial.println("Перегенерація масивів для побудови дерев...");
    // Генеруємо нові випадкові значення, оскільки з відсортованих масивів дерево вийде незбалансованим
    for (int i = 0; i < SIZE_INT; i++) dataInt[i] = esp_random() % 1000;
    for (int i = 0; i < SIZE_DOUBLE; i++) dataDouble[i] = (double)(esp_random() % 10000) / 100.0;
    for (int i = 0; i < SIZE_CHAR; i++) dataChar[i] = 'a' + (esp_random() % 26);
    for (int i = 0; i < SIZE_FLOAT; i++) dataFloat[i] = (float)(esp_random() % 1000) / 10.0f;

    // 1. Дерево INT
    BstNode<int>* treeInt = nullptr; // Створюємо порожній корінь
    heapStart = ESP.getFreeHeap();
    timerStart = micros();
    for (int i = 0; i < SIZE_INT; i++) {
        treeInt = addNodeToTree(treeInt, dataInt[i]);
    }
    timerEnd = micros();
    durationUs = timerEnd - timerStart;
    heapEnd = ESP.getFreeHeap();
    memoryConsumed = heapStart - heapEnd;

    Serial.printf("Дерево INT побудовано: %lu мкс | Витрачено: %u байт\n", durationUs, memoryConsumed);
    timeStats[4] = durationUs; memStats[4] = memoryConsumed;

    // 2. Дерево DOUBLE
    BstNode<double>* treeDouble = nullptr;
    heapStart = ESP.getFreeHeap();
    timerStart = micros();
    for (int i = 0; i < SIZE_DOUBLE; i++) {
        treeDouble = addNodeToTree(treeDouble, dataDouble[i]);
    }
    timerEnd = micros();
    durationUs = timerEnd - timerStart;
    heapEnd = ESP.getFreeHeap();
    memoryConsumed = heapStart - heapEnd;

    Serial.printf("Дерево DOUBLE побудовано: %lu мкс | Витрачено: %u байт\n", durationUs, memoryConsumed);
    timeStats[5] = durationUs; memStats[5] = memoryConsumed;

    // 3. Дерево FLOAT
    BstNode<float>* treeFloat = nullptr;
    heapStart = ESP.getFreeHeap();
    timerStart = micros();
    for (int i = 0; i < SIZE_FLOAT; i++) {
        treeFloat = addNodeToTree(treeFloat, dataFloat[i]);
        if (i % 50 == 0) yield(); // Захист від спрацювання Watchdog Timer при великих об'ємах
    }
    timerEnd = micros();
    durationUs = timerEnd - timerStart;
    heapEnd = ESP.getFreeHeap();
    memoryConsumed = heapStart - heapEnd;

    Serial.printf("Дерево FLOAT побудовано: %lu мкс | Витрачено: %u байт\n", durationUs, memoryConsumed);
    timeStats[6] = durationUs; memStats[6] = memoryConsumed;

    // 4. Дерево CHAR
    BstNode<char>* treeChar = nullptr;
    heapStart = ESP.getFreeHeap();
    timerStart = micros();
    for (int i = 0; i < SIZE_CHAR; i++) {
        treeChar = addNodeToTree(treeChar, dataChar[i]);
        if (i % 50 == 0) yield();
    }
    timerEnd = micros();
    durationUs = timerEnd - timerStart;
    heapEnd = ESP.getFreeHeap();
    memoryConsumed = heapStart - heapEnd;

    Serial.printf("Дерево CHAR побудовано: %lu мкс | Витрачено: %u байт\n", durationUs, memoryConsumed);
    timeStats[7] = durationUs; memStats[7] = memoryConsumed;

    // --- ЕТАП 5: ФІНАЛЬНИЙ ЗВІТ ДЛЯ ПОБУДОВИ ГРАФІКІВ [cite: 14] ---
    Serial.println("\n================ ФІНАЛЬНА СТАТИСТИКА ================");
    Serial.println("Тип, Розмір, Операція, Час(мкс), Пам'ять(байт)");

    String typeNames[] = { "Int", "Double", "Char", "Float" };
    int arraySizes[] = { SIZE_INT, SIZE_DOUBLE, SIZE_CHAR, SIZE_FLOAT };

    Serial.printf("Поточна частота ядра: %d MHz\n", getCpuFrequencyMhz());

    // Блок виводу результатів сортування
    for (int i = 0; i < 4; i++) {
        Serial.printf("%s, %d, FastSort, %lu, %u\n", typeNames[i].c_str(), arraySizes[i], timeStats[i], memStats[i]);
    }
    // Блок виводу результатів дерев
    for (int i = 0; i < 4; i++) {
        Serial.printf("%s, %d, BinaryTree, %lu, %u\n", typeNames[i].c_str(), arraySizes[i], timeStats[i + 4], memStats[i + 4]);
    }
    Serial.println("=====================================================");
}

// ********************** ГОЛОВНИЙ ЦИКЛ (ПОРОЖНІЙ) ***********************************
void loop() {
    // В даній лабораторній роботі весь функціонал виконується одноразово у блоці setup()
}