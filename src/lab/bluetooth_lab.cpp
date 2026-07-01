#include <Arduino.h>
#include <NimBLEDevice.h>

#include "bluetooth_lab.h"
#include "system_log.h"

static String bleName = "idDOS";

static bool bleReady = false;
static bool serverRunning = false;
static bool advertisingRunning = false;

static NimBLEServer* server = nullptr;
static NimBLEService* service = nullptr;
static NimBLECharacteristic* txChar = nullptr;
static NimBLECharacteristic* rxChar = nullptr;

static const char* SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char* RX_UUID      = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
static const char* TX_UUID      = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

void bleBegin()
{
    if (bleReady) return;

    NimBLEDevice::init(bleName.c_str());
    NimBLEDevice::setDeviceName(bleName.c_str());
    NimBLEDevice::setPower(ESP_PWR_LVL_P3);

    bleReady = true;
}

bool bleServerStart()
{
    bleBegin();

    if (serverRunning && server && service && rxChar && txChar) return true;

    server = NimBLEDevice::createServer();
    if (!server) return false;

    service = server->createService(SERVICE_UUID);
    if (!service) return false;

    rxChar = service->createCharacteristic(RX_UUID, NIMBLE_PROPERTY::WRITE);
    if (!rxChar) return false;

    txChar = service->createCharacteristic(TX_UUID, NIMBLE_PROPERTY::NOTIFY);
    if (!txChar) return false;

    service->start();
    serverRunning = true;
    systemLog("BLE SERVER START");
    return true;
}

void bleAdvertisingStop()
{
    if (!bleReady) {
        advertisingRunning = false;
        return;
    }

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    if (adv) adv->stop();
    advertisingRunning = false;
}

void bleServerStop()
{
    bleAdvertisingStop();
    serverRunning = false;
    advertisingRunning = false;
    systemLog("BLE SOFT STOP");
}

void bleFullStop()
{
    if (bleReady) {
        NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
        if (adv) adv->stop();
        NimBLEDevice::deinit(true);
    }

    server = nullptr;
    service = nullptr;
    txChar = nullptr;
    rxChar = nullptr;

    bleReady = false;
    serverRunning = false;
    advertisingRunning = false;
    systemLog("BLE FULL STOP");
    delay(250);
}

bool bleServerRunning()
{
    return serverRunning;
}

bool bleAdvertisingStart()
{
    if (!serverRunning) {
        if (!bleServerStart()) return false;
    }

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    if (!adv) return false;

    adv->stop();
    adv->reset();
    adv->addServiceUUID(SERVICE_UUID);
    adv->setName(bleName.c_str());

    if (!adv->start()) {
        advertisingRunning = false;
        return false;
    }

    advertisingRunning = true;
    systemLog("BLE ADV START");
    return true;
}

bool bleAdvertisingRunning()
{
    return advertisingRunning;
}

String bleDeviceName()
{
    return bleName;
}

void bleSetDeviceName(const String& name)
{
    String n = name;
    n.trim();

    if (n.length() == 0) return;
    if (n.length() > 20) n = n.substring(0, 20);

    if (serverRunning || advertisingRunning) return;

    bleName = n;

    if (bleReady) {
        NimBLEDevice::setDeviceName(bleName.c_str());
    }

    systemLog("BLE NAME " + bleName);
}

void bleStopForWifi()
{
    bleFullStop();
}

String bleStatusText()
{
    if (advertisingRunning) return "ADVERTISING";
    if (serverRunning) return "SERVER ON";
    return "OFF";
}
