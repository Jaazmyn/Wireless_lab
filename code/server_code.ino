#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// TODO: add new global variables for your sensor readings and processed data
#define TRIGGER_PIN D0
#define ECHO_PIN D1
const int filterSize = 5;  // Moving average window
float distanceBuffer[filterSize] = {0};  // Buffer for past readings
int bufferIndex = 0;  // Circular buffer index

// TODO: Change the UUID to your own (any specific one works, but make sure they're different from others'). You can generate one here: https://www.uuidgenerator.net/
#define SERVICE_UUID        "0d40473e-9b72-4421-b5d8-ee5f1a9b8692"
#define CHARACTERISTIC_UUID "61e012d4-d5ea-4382-8ddd-1c13a06f14c9"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

// TODO: add DSP algorithm functions here
// Moving average filter
float movingAverageFilter(float newDistance) {
    distanceBuffer[bufferIndex] = newDistance;
    bufferIndex = (bufferIndex + 1) % filterSize;  // Circular buffer index

    float sum = 0;
    for (int i = 0; i < filterSize; i++) {
        sum += distanceBuffer[i];
    }
    return sum / filterSize;
}

// Function to read ultrasonic sensor distance
float readDistance() {
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.034 / 2;  // Convert to cm
    return distance;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // TODO: add codes for handling your sensor setup (pinMode, etc.)
     pinMode(TRIGGER_PIN, OUTPUT);
     pinMode(ECHO_PIN, INPUT);

    // TODO: name your device to avoid conflictions
    BLEDevice::init("Jaz_Laura");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();
    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // TODO: add codes for handling your sensor readings (analogRead, etc.)
    unsigned long currentMillis = millis();

    if (deviceConnected && currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        float rawDistance = readDistance();  // Read raw sensor data
        float filteredDistance = movingAverageFilter(rawDistance);  // Apply filter

    // TODO: use your defined DSP algorithm to process the readings
    Serial.print("Raw Distance: ");
    Serial.print(rawDistance);
    Serial.print(" cm | Filtered Distance: ");
    Serial.print(filteredDistance);
    Serial.println(" cm");

    
    if (deviceConnected) {
        // Send new readings to database
        // TODO: change the following code to send your own readings and processed data
        if (filteredDistance < 30.0) {  // Transmit only if distance < 30 cm
            String distanceStr = String(filteredDistance);
            pCharacteristic->setValue(distanceStr.c_str());
            pCharacteristic->notify();
            Serial.println("Notify value: " + distanceStr + " cm");
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}
}