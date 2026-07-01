#pragma once
#include <Arduino.h>

struct WifiEntry {
    String ssid;
    int32_t rssi;
    String security;
    int32_t channel;
    String bssid;
};

void networkBegin();

int networkScan();
int networkCount();
WifiEntry networkGet(int index);

bool networkConnect(const String& ssid, const String& password);
void networkDisconnect();

bool networkConnected();
String networkCurrentSSID();
String networkIP();
String networkGateway();
String networkSubnet();
String networkDNS();
int32_t networkCurrentRSSI();

int networkQualityFromRSSI(int32_t rssi);