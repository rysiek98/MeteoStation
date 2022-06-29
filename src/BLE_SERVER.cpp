/*
  Code based on:
    Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

Modified by Michał Ryszka
*/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "DHT.h"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
float temperature = 0.0;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};

void setup()
{
    Serial.begin(115200);

    // Create the BLE Device
    BLEDevice::init("ESP32");
    dht.begin();
    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);

    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    // Create a BLE Descriptor
    pCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    Serial.println("Waiting a client connection to notify...");
}

void loop()
{
    // notify changed value
    if (deviceConnected)
    {
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();
        Serial.printf("Measured temp : %f and humidity %f", temperature, humidity);
        char temperatureToChar[5];
        dtostrf(temperature, 5, 2, temperatureToChar);
        char humiditiyToChar[5];
        dtostrf(humidity, 5, 2, humiditiyToChar);
        char packetToSend[12];
        packetToSend[0] = 't';
        for (int i = 1; i < 12; i++)
        {
            if (i < 6)
            {
                packetToSend[i] = temperatureToChar[i - 1];
            }
            else if (i == 6)
            {
                packetToSend[i] = 'h';
            }
            else
            {
                packetToSend[i] = humiditiyToChar[i - 7];
            }
        }
        Serial.println();
        for (int i = 0; i < 12; i++)
        {
            Serial.print(packetToSend[i]);
        }
        pCharacteristic->setValue((uint8_t *)packetToSend, 12);
        pCharacteristic->notify();
        delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500);                  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected)
    {
        Serial.println("Connecting...");
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    if (!deviceConnected)
    {
        Serial.println("Nobody connect to the server...");
    }

    delay(5000);
}