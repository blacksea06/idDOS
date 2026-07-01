#include <Arduino.h>
#include <SD.h>
#include <mbedtls/sha256.h>
#include <esp_system.h>

#include "config.h"
#include "storage.h"

static const char* CONFIG_PATH = "/data/config.sys";
static const char* CONFIG_TMP  = "/data/config.tmp";

static String cfgUsername = "USER";
static String cfgSalt = "";
static String cfgPassHash = "";
static bool cfgLegacyMigrated = false;

static String cleanValue(String v)
{
    v.trim();
    v.replace("\r", "");
    v.replace("\n", "");
    return v;
}

static String sanitizeUsername(String v)
{
    v = cleanValue(v);
    String out;
    out.reserve(16);

    for (uint32_t i = 0; i < v.length() && out.length() < 16; ++i) {
        char c = v[i];
        if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-') {
            out += c;
        }
    }

    if (out.length() == 0) out = "USER";
    return out;
}

static String sanitizePassword(String v)
{
    v = cleanValue(v);
    if (v.length() > 64) v = v.substring(0, 64);
    return v;
}

static String byteToHex(uint8_t b)
{
    const char* hex = "0123456789abcdef";
    String s;
    s += hex[(b >> 4) & 0x0F];
    s += hex[b & 0x0F];
    return s;
}

static String randomSaltHex()
{
    String salt;
    salt.reserve(32);
    for (int i = 0; i < 16; ++i) {
        salt += byteToHex((uint8_t)(esp_random() & 0xFF));
    }
    return salt;
}

static String sha256Hex(const String& data)
{
    uint8_t out[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx,
                          reinterpret_cast<const unsigned char*>(data.c_str()),
                          data.length());
    mbedtls_sha256_finish(&ctx, out);
    mbedtls_sha256_free(&ctx);

    String hex;
    hex.reserve(64);
    for (uint8_t b : out) hex += byteToHex(b);
    return hex;
}

static String passwordHash(const String& password, const String& salt)
{
    return sha256Hex("idDOS:v2:" + salt + ":" + password);
}

static String readLimitedLine(File& f, size_t limit)
{
    String out;
    out.reserve(limit < 96 ? limit : 96);

    while (f.available()) {
        char c = (char)f.read();
        if (c == '\n') break;
        if (c == '\r') continue;
        if (out.length() < limit) out += c;
    }

    return out;
}

static bool constantTimeEqual(const String& a, const String& b)
{
    uint8_t diff = (uint8_t)(a.length() ^ b.length());
    uint32_t maxLen = a.length() > b.length() ? a.length() : b.length();

    for (uint32_t i = 0; i < maxLen; ++i) {
        uint8_t ca = (i < a.length()) ? (uint8_t)a[i] : 0;
        uint8_t cb = (i < b.length()) ? (uint8_t)b[i] : 0;
        diff |= ca ^ cb;
    }

    return diff == 0;
}

void configLoad()
{
    cfgUsername = "USER";
    cfgSalt = "";
    cfgPassHash = "";
    cfgLegacyMigrated = false;

    if (!storageReady()) return;

    if (!SD.exists("/data")) SD.mkdir("/data");

    if (!SD.exists(CONFIG_PATH)) {
        configSave();
        return;
    }

    File f = SD.open(CONFIG_PATH, FILE_READ);
    if (!f) return;

    String legacyPassword = "";
    String auth = "";

    while (f.available()) {
        String line = readLimitedLine(f, 160);
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        int eq = line.indexOf('=');
        if (eq < 0) continue;

        String key = line.substring(0, eq);
        String val = cleanValue(line.substring(eq + 1));
        key.toUpperCase();

        if (key == "USERNAME") cfgUsername = sanitizeUsername(val);
        else if (key == "AUTH") auth = val;
        else if (key == "SALT") cfgSalt = val;
        else if (key == "PASSHASH") cfgPassHash = val;
        else if (key == "PASSWORD") legacyPassword = val;
    }

    f.close();

    if (legacyPassword.length() > 0) {
        configSetPassword(legacyPassword);
        cfgLegacyMigrated = true;
        configSave();
        return;
    }

    if (cfgSalt.length() != 32 || cfgPassHash.length() != 64) {
        cfgSalt = "";
        cfgPassHash = "";
    }

    if (auth.length() == 0 && cfgPassHash.length() == 0) {
        configSave();
    }
}

void configSave()
{
    if (!storageReady()) return;

    if (!SD.exists("/data")) SD.mkdir("/data");
    if (SD.exists(CONFIG_TMP)) SD.remove(CONFIG_TMP);

    File f = SD.open(CONFIG_TMP, FILE_WRITE);
    if (!f) return;

    f.println("CONFIG_VERSION=2");
    f.println("USERNAME=" + sanitizeUsername(cfgUsername));

    if (cfgPassHash.length() == 64 && cfgSalt.length() == 32) {
        f.println("AUTH=SHA256");
        f.println("SALT=" + cfgSalt);
        f.println("PASSHASH=" + cfgPassHash);
    } else {
        f.println("AUTH=NONE");
    }

    f.close();

    if (SD.exists(CONFIG_PATH)) SD.remove(CONFIG_PATH);
    SD.rename(CONFIG_TMP, CONFIG_PATH);
}

String configUsername()
{
    return sanitizeUsername(cfgUsername);
}

String configPassword()
{
    return "";
}

String configPasswordMasked()
{
    if (!configHasPassword()) return "<EMPTY>";
    return "********";
}

void configSetUsername(const String& v)
{
    cfgUsername = sanitizeUsername(v);
}

void configSetPassword(const String& v)
{
    String p = sanitizePassword(v);
    if (p.length() == 0) {
        configClearPassword();
        return;
    }

    cfgSalt = randomSaltHex();
    cfgPassHash = passwordHash(p, cfgSalt);
}

void configClearPassword()
{
    cfgSalt = "";
    cfgPassHash = "";
}

bool configHasPassword()
{
    return cfgSalt.length() == 32 && cfgPassHash.length() == 64;
}

bool configCheckPassword(const String& input)
{
    if (!configHasPassword()) return true;
    String p = sanitizePassword(input);
    String h = passwordHash(p, cfgSalt);
    return constantTimeEqual(h, cfgPassHash);
}
