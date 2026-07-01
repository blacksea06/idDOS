#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <algorithm>
#include "network.h"

static std::vector<WifiEntry> nets;
static constexpr int WIFI_SCAN_MAX = 24;

static String securityName(wifi_auth_mode_t mode)
{
    switch (mode)
    {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "SECURE";
    }
}

void networkBegin()
{
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
}

int networkScan()
{
    nets.clear();
    nets.reserve(WIFI_SCAN_MAX);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    int count = WiFi.scanNetworks(false, true);
    if (count <= 0) {
        WiFi.scanDelete();
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        WifiEntry e;
        e.ssid = WiFi.SSID(i);
        e.rssi = WiFi.RSSI(i);
        e.channel = WiFi.channel(i);
        e.bssid = WiFi.BSSIDstr(i);
        e.security = securityName(WiFi.encryptionType(i));

        bool duplicate = false;
        for (auto& existing : nets) {
            if (existing.ssid == e.ssid && existing.bssid == e.bssid) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate) nets.push_back(e);
    }

    WiFi.scanDelete();

    std::sort(nets.begin(), nets.end(), [](const WifiEntry& a, const WifiEntry& b) {
        return a.rssi > b.rssi;
    });

    if ((int)nets.size() > WIFI_SCAN_MAX) {
        nets.resize(WIFI_SCAN_MAX);
    }

    return (int)nets.size();
}

int networkCount()
{
    return (int)nets.size();
}

WifiEntry networkGet(int index)
{
    if (index < 0 || index >= (int)nets.size())
    {
        return WifiEntry{};
    }

    return nets[index];
}

bool networkConnect(const String& ssid, const String& password)
{
    WiFi.disconnect(true, true);
    delay(50);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    if (password.length() == 0) WiFi.begin(ssid.c_str());
    else WiFi.begin(ssid.c_str(), password.c_str());

    uint32_t start = millis();

    while (millis() - start < 15000)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            return true;
        }

        delay(100);
    }

    return false;
}

void networkDisconnect()
{
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
}

bool networkConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

String networkCurrentSSID()
{
    return WiFi.SSID();
}

String networkIP()
{
    return WiFi.localIP().toString();
}

String networkGateway()
{
    return WiFi.gatewayIP().toString();
}

String networkSubnet()
{
    return WiFi.subnetMask().toString();
}

String networkDNS()
{
    return WiFi.dnsIP().toString();
}

int32_t networkCurrentRSSI()
{
    if (!networkConnected()) return 0;
    return WiFi.RSSI();
}

int networkQualityFromRSSI(int32_t rssi)
{
    if (rssi <= -100) return 0;
    if (rssi >= -50) return 100;
    return 2 * (rssi + 100);
}