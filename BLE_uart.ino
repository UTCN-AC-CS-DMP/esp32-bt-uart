/*
 * Based on ESP32 BLE UART example
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE/examples/BLE_uart
 */
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// NOTE: Leave these default values for Nordic nRF chip, so that the
//       terminal emulator on cellphones won't have to be configured
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NNRF_RX_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define NNRF_TX_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Change this to something unique
const String BLE_NETWORK_NAME = "DMP BLE Device";

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    const std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.print("XX | RECV: ");
      if (rxValue.back() == '\n') Serial.print(rxValue.c_str());
      else Serial.println(rxValue.c_str());
    }
  }
};

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init(BLE_NETWORK_NAME.c_str());

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Init / create a BLE Characteristics
  pTxCharacteristic = pService->createCharacteristic(
    NNRF_TX_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    NNRF_RX_UUID,
    BLECharacteristic::PROPERTY_WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  printIntro();
}

void loop() {

  if (deviceConnected && Serial.available()) {
    sendMessage();
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                     // give the bluetooth stack the chance to get things ready
    BLEDevice::startAdvertising();  // restart advertising
    Serial.println("INFO: Client disconnected, restarted advertising!");
    oldDeviceConnected = deviceConnected;
  }

  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("INFO: New device connected!");
    oldDeviceConnected = deviceConnected;
  }
}

void sendMessage() {
  const String LINE_READ = Serial.readString();
  if (LINE_READ.length() > 0) {
    uint8_t *C_BASED_STR = (uint8_t *)LINE_READ.c_str();
    pTxCharacteristic->setValue(C_BASED_STR, LINE_READ.length());
    pTxCharacteristic->notify();
    Serial.print(String("YY | SENT: ") + LINE_READ);
  }
}

void printIntro() {
  Serial.println("-------------------------------------");
  Serial.println("| DMP BLE UART App started on ESP32 |");
  Serial.println("| _________________________________ |");
  Serial.println("| connect to it using a device that |");
  Serial.println("| supports BLE (Bluetooth ver 4.0+) |");
  Serial.println("-------------------------------------");
}
