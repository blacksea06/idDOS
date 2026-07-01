#include "shell_internal.h"

// WLAN
// ───────────────────────────────────────────────────────────────────
String wlanRowText(const WifiEntry& e) {
    String ssid = e.ssid.length() ? e.ssid : "<hidden>";
    ssid = trimToWidth(ssid, 14);
    return padRight(ssid, 15) + String(e.rssi) + " " + e.security;
}

void showWlanList(bool doScan) {
    S.uiMode = UI_WLAN_LIST;
    S.section = LAB_SECTION;

    if (doScan) {
        M5Cardputer.Display.fillScreen(BLACK);
        drawStatusBar();
        printAt(5, Dim::HIST_Y0, "WLAN NETWORKS", CYAN);
        printAt(5, 46, "SCANNING...", YELLOW);

        S.wlanCount = networkScan();
        S.wlanSel = 0;
        S.wlanScroll = 0;
    } else {
        S.wlanCount = networkCount();
    }

    if (S.wlanCount < 0) S.wlanCount = 0;

    if (S.wlanSel < 0) S.wlanSel = 0;
    if (S.wlanSel >= S.wlanCount && S.wlanCount > 0) S.wlanSel = S.wlanCount - 1;

    if (S.wlanSel < S.wlanScroll) S.wlanScroll = S.wlanSel;
    if (S.wlanSel >= S.wlanScroll + Limit::WLAN_VISIBLE) {
        S.wlanScroll = S.wlanSel - Limit::WLAN_VISIBLE + 1;
    }

    int maxScroll = S.wlanCount - Limit::WLAN_VISIBLE;
    if (maxScroll < 0) maxScroll = 0;
    if (S.wlanScroll < 0) S.wlanScroll = 0;
    if (S.wlanScroll > maxScroll) S.wlanScroll = maxScroll;

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    printAt(5, Dim::HIST_Y0, "WLAN NETWORKS", CYAN);

    if (S.wlanCount <= 0) {
        printAt(5, 44, "NO NETWORKS FOUND", RED);
        printAt(5, 62, "ENTER rescan", DARKGREY);
    } else {
        for (int row = 0; row < Limit::WLAN_VISIBLE; row++) {
            int idx = S.wlanScroll + row;
            if (idx >= S.wlanCount) break;

            WifiEntry e = networkGet(idx);
            int y = Dim::HIST_Y0 + 18 + row * 15;
            bool sel = (S.wlanSel == idx);

            printAt(5, y, sel ? ">" : " ", GREEN);
            printAt(18, y, trimToWidth(wlanRowText(e), 34), sel ? GREEN : WHITE);
        }
    }

    drawMenuScrollbar(S.wlanCount, Limit::WLAN_VISIBLE, S.wlanSel, Dim::HIST_Y0 + 18, 76);

    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, Dim::PROMPT_Y, "ENTER info  ESC lab  FN+ENTER scan", WHITE);
}

WifiEntry selectedWifi() {
    if (S.selectedNet < 0) return WifiEntry{};
    return networkGet(S.selectedNet);
}

int wlanInfoTotalRows() {
    WifiEntry e = selectedWifi();
    bool connected = networkConnected() && e.ssid.length() && (networkCurrentSSID() == e.ssid);
    return connected ? 9 : 7;
}

String wlanInfoLine(int row) {
    WifiEntry e = selectedWifi();
    bool connected = networkConnected() && e.ssid.length() && (networkCurrentSSID() == e.ssid);

    if (row == 0) return connected ? "DISCONNECT" : "CONNECT";
    if (row == 1) return "SSID     " + trimToWidth(e.ssid.length() ? e.ssid : "<hidden>", 22);

    if (connected) {
        if (row == 2) return "IP       " + networkIP();
        if (row == 3) return "GATEWAY  " + networkGateway();
        if (row == 4) return "DNS      " + networkDNS();
        if (row == 5) return "RSSI     " + String(networkCurrentRSSI()) + " dBm";
        if (row == 6) return "QUALITY  " + String(networkQualityFromRSSI(networkCurrentRSSI())) + "%";
        if (row == 7) return "SECURITY " + e.security;
        if (row == 8) return "CHANNEL  " + String(e.channel);
    } else {
        if (row == 2) return "RSSI     " + String(e.rssi) + " dBm";
        if (row == 3) return "QUALITY  " + String(networkQualityFromRSSI(e.rssi)) + "%";
        if (row == 4) return "SECURITY " + e.security;
        if (row == 5) return "CHANNEL  " + String(e.channel);
        if (row == 6) return "BSSID    " + e.bssid;
    }

    return "";
}

void showWlanInfo() {
    S.uiMode = UI_WLAN_INFO;
    S.section = LAB_SECTION;

    int total = wlanInfoTotalRows();

    if (S.wlanInfoScroll < 0) S.wlanInfoScroll = 0;

    int maxScroll = total - Limit::WLAN_INFO_VISIBLE;
    if (maxScroll < 0) maxScroll = 0;
    if (S.wlanInfoScroll > maxScroll) S.wlanInfoScroll = maxScroll;

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    WifiEntry e = selectedWifi();
    String title = e.ssid.length() ? e.ssid : "<hidden>";

    printAt(5, Dim::HIST_Y0, trimToWidth(title, 28), CYAN);

    for (int row = 0; row < Limit::WLAN_INFO_VISIBLE; row++) {
        int idx = S.wlanInfoScroll + row;
        if (idx >= total) break;

        int y = Dim::HIST_Y0 + 18 + row * 15;

        if (idx == 0) {
            printAt(5, y, ">", GREEN);
            printAt(18, y, wlanInfoLine(idx), GREEN);
        } else {
            printAt(18, y, trimToWidth(wlanInfoLine(idx), 34), WHITE);
        }
    }

    drawMenuScrollbar(total, Limit::WLAN_INFO_VISIBLE, S.wlanInfoScroll, Dim::HIST_Y0 + 18, 76);

    printAt(0, Dim::SEP_Y, "----------------------------------------", DARKGREY);
    printAt(5, Dim::PROMPT_Y, "ENTER action  ESC list  UP/DOWN", WHITE);
}

void startWlanPassword() {
    S.uiMode = UI_WLAN_PASS;
    S.wlanPassword = "";

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    WifiEntry e = selectedWifi();

    printAt(5, Dim::HIST_Y0, "PASSWORD FOR", CYAN);
    printAt(5, 35, trimToWidth(e.ssid, 34), WHITE);
    printAt(5, 61, ">", GREEN);
    printAt(18, 61, "_", WHITE);
    printAt(5, Dim::PROMPT_Y, "ENTER CONNECT  ESC BACK", DARKGREY);
}

void renderWlanPassword() {
    M5Cardputer.Display.fillRect(0, 55, Dim::SCREEN_W, 25, BLACK);
    printAt(5, 61, ">", GREEN);
    String masked = "";
    for (int i = 0; i < S.wlanPassword.length(); ++i) masked += '*';
    printAt(18, 61, trimToWidth(masked + "_", 34), WHITE);
}

void connectSelectedWlan() {
    WifiEntry e = selectedWifi();

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    printAt(5, Dim::HIST_Y0, "CONNECTING...", CYAN);
    printAt(5, 40, trimToWidth(e.ssid, 34), WHITE);

    bool ok = networkConnect(e.ssid, S.wlanPassword);

    M5Cardputer.Display.fillScreen(BLACK);
    drawStatusBar();

    if (ok) {
        printAt(5, Dim::HIST_Y0, "CONNECTED", GREEN);
        printAt(5, 40, "IP " + networkIP(), WHITE);
        soundSuccess();
    } else {
        printAt(5, Dim::HIST_Y0, "CONNECT FAIL", RED);
        printAt(5, 40, "CHECK PASSWORD", YELLOW);
        soundError();
    }

    delay(900);

    S.wlanPassword = "";
    S.wlanInfoScroll = 0;
    showWlanInfo();
}

// ───────────────────────────────────────────────────────────────────
// SD FILE OPS
// ───────────────────────────────────────────────────────────────────
