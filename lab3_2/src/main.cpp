#include <Arduino.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"

// UUIDs for our service and characteristic
const char* SERVICE_UUID = "00000000-0000-0000-0000-100000000000";
const char* CHARACTERISTIC_UUID_RX = "00000000-0000-0000-0000-100000000001";
const char* CHARACTERISTIC_UUID_TX = "00000000-0000-0000-0000-100000000010";

BLEServer *pServer = NULL;
BLEService *pService = NULL;
BLECharacteristic *pTxCharacteristic = NULL;
BLECharacteristic *pRxCharacteristic = NULL;

bool deviceConnected = false;
bool wasConnected = false;

// Callback class for handling server events
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
    }
};

// Callback class for handling received data
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        Serial.print("Received: ");
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Setting up BLE UART...");

  // Initialize BLE device
  BLEDevice::init("ESP32-UART");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics
  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxCharacteristic->addDescriptor(new BLE2902());

  pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.printf("BLE UART is ready. MAC: %s\n", BLEDevice::getAddress().toString().c_str());
}

void loop() {
  if (deviceConnected) {
    if (Serial.available()) {
      String message = Serial.readStringUntil('\n');
      pTxCharacteristic->setValue(message.c_str());
      pTxCharacteristic->notify();
      Serial.print("Sent: ");
      Serial.println(message);
    }
  }
  
  if (!deviceConnected && wasConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Started advertising again");
    wasConnected = deviceConnected;
  }
  
  if (deviceConnected && !wasConnected) {
    wasConnected = deviceConnected;
  }
  
  delay(20);
}