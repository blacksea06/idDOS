#include <Arduino.h>
#include <M5Cardputer.h>
#include <SD.h>

#include "audio_lab.h"
#include "storage.h"
#include "system_log.h"

namespace AudioLab {

static const char* AUDIO_DIR = "/audio";
static const char* TMP_PATH  = "/audio/.tmp_rec.wav";

static constexpr size_t REC_SAMPLES = 512;
static constexpr size_t PLAY_SAMPLES = 1024;

static bool audioReady = false;
static bool recording = false;
static bool tmpReady = false;

static File recFile;
static uint32_t recStartMs = 0;
static uint32_t recBytesWritten = 0;
static int16_t recBuf[REC_SAMPLES];
static int16_t level = 0;

static File playFile;
static bool playing = false;
static String playPath = "";
static String playBaseName = "";
static uint32_t playSampleRate = WAV_SAMPLE_RATE;
static uint16_t playChannels = WAV_CHANNELS;
static uint16_t playBits = WAV_BITS;
static uint32_t playDataStart = 44;
static uint32_t playDataBytes = 0;
static uint32_t playReadBytes = 0;
static int16_t playBuf[PLAY_SAMPLES];
static uint8_t playbackVolume = 180;

static void writeU16(File& f, uint16_t v)
{
    f.write((uint8_t)(v & 0xFF));
    f.write((uint8_t)((v >> 8) & 0xFF));
}

static void writeU32(File& f, uint32_t v)
{
    f.write((uint8_t)(v & 0xFF));
    f.write((uint8_t)((v >> 8) & 0xFF));
    f.write((uint8_t)((v >> 16) & 0xFF));
    f.write((uint8_t)((v >> 24) & 0xFF));
}

static uint16_t readU16(File& f)
{
    uint16_t v = 0;
    v |= (uint16_t)f.read();
    v |= (uint16_t)f.read() << 8;
    return v;
}

static uint32_t readU32(File& f)
{
    uint32_t v = 0;
    v |= (uint32_t)f.read();
    v |= (uint32_t)f.read() << 8;
    v |= (uint32_t)f.read() << 16;
    v |= (uint32_t)f.read() << 24;
    return v;
}

static void writeWavHeader(File& f, uint32_t dataBytes)
{
    uint32_t byteRate = WAV_SAMPLE_RATE * WAV_CHANNELS * (WAV_BITS / 8);
    uint16_t blockAlign = WAV_CHANNELS * (WAV_BITS / 8);

    f.seek(0);
    f.write((const uint8_t*)"RIFF", 4);
    writeU32(f, 36 + dataBytes);
    f.write((const uint8_t*)"WAVE", 4);

    f.write((const uint8_t*)"fmt ", 4);
    writeU32(f, 16);
    writeU16(f, 1);               // PCM
    writeU16(f, WAV_CHANNELS);
    writeU32(f, WAV_SAMPLE_RATE);
    writeU32(f, byteRate);
    writeU16(f, blockAlign);
    writeU16(f, WAV_BITS);

    f.write((const uint8_t*)"data", 4);
    writeU32(f, dataBytes);
}

static bool parseWavHeader(File& f)
{
    char tag[5] = {0};

    f.seek(0);
    if (f.readBytes(tag, 4) != 4 || String(tag) != "RIFF") return false;
    (void)readU32(f);
    memset(tag, 0, sizeof(tag));
    if (f.readBytes(tag, 4) != 4 || String(tag) != "WAVE") return false;

    bool gotFmt = false;
    bool gotData = false;

    while (f.available()) {
        memset(tag, 0, sizeof(tag));
        if (f.readBytes(tag, 4) != 4) break;
        uint32_t chunkSize = readU32(f);
        uint32_t chunkStart = f.position();
        String chunk = String(tag);

        if (chunk == "fmt ") {
            uint16_t audioFormat = readU16(f);
            playChannels = readU16(f);
            playSampleRate = readU32(f);
            (void)readU32(f); // byte rate
            (void)readU16(f); // block align
            playBits = readU16(f);
            gotFmt = (audioFormat == 1 && playBits == 16 && (playChannels == 1 || playChannels == 2));
        } else if (chunk == "data") {
            playDataStart = f.position();
            playDataBytes = chunkSize;
            gotData = true;
            break;
        }

        f.seek(chunkStart + chunkSize);
    }

    return gotFmt && gotData && playDataBytes > 0;
}

static int16_t calcLevel(const int16_t* data, size_t samples)
{
    int32_t peak = 0;
    for (size_t i = 0; i < samples; ++i) {
        int32_t v = data[i];
        if (v < 0) v = -v;
        if (v > peak) peak = v;
    }
    if (peak > 32767) peak = 32767;
    return (int16_t)peak;
}

static void enterPlaybackMode()
{
    // Cardputer-Adv uses a shared codec/I2S audio path. After Mic.record()
    // some M5Cardputer/M5Unified versions leave the codec in capture mode.
    // For WAV playback we explicitly shut down the mic path and re-open
    // the speaker path before feeding PCM chunks.
    recording = false;
    M5Cardputer.Mic.end();
    audioReady = false;
    delay(80);

    M5Cardputer.Speaker.end();
    delay(30);
    M5Cardputer.Speaker.begin();
    M5Cardputer.Speaker.setVolume(playbackVolume);
    M5Cardputer.Speaker.stop();
    delay(80);
}

static void enterCaptureMode()
{
    M5Cardputer.Speaker.stop();
    delay(20);
    audioReady = M5Cardputer.Mic.begin();
}

static String cleanBaseName(String name)
{
    name.trim();
    name.replace("\\", "");
    name.replace("/", "");
    name.replace("..", "");
    name.replace(" ", "_");

    String out;
    out.reserve(24);
    for (uint32_t i = 0; i < name.length() && out.length() < 24; ++i) {
        char c = name[i];
        if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.') {
            out += c;
        }
    }

    if (out.length() == 0) out = "rec";
    return out;
}

static String uniquePath(const String& preferred)
{
    if (!SD.exists(preferred)) return preferred;

    int slash = preferred.lastIndexOf('/');
    String dir = (slash >= 0) ? preferred.substring(0, slash + 1) : "/";
    String name = (slash >= 0) ? preferred.substring(slash + 1) : preferred;

    int dot = name.lastIndexOf('.');
    String base = (dot > 0) ? name.substring(0, dot) : name;
    String ext = (dot > 0) ? name.substring(dot) : "";

    for (int i = 1; i < 100; ++i) {
        String p = dir + base + "_" + String(i) + ext;
        if (!SD.exists(p)) return p;
    }

    return dir + base + "_new" + ext;
}

bool begin()
{
    if (audioReady) return true;

    // M5Cardputer-Adv uses ES8311 + I2S. M5Cardputer.Mic initializes codec/I2S.
    // Do not keep forcing Speaker.begin() here; recording/live-level mode owns
    // the codec input path. Playback will explicitly switch to speaker mode.
    audioReady = M5Cardputer.Mic.begin();
    if (audioReady) {
        systemLog("AUDIO READY");
    } else {
        systemLog("AUDIO INIT FAIL");
    }
    return audioReady;
}

bool available()
{
    return audioReady;
}

void idleSilence()
{
    // Put the shared ES8311/I2S audio path into a quiet idle state.
    // This is used after REC save/discard and after playback stop to avoid
    // the low background hum that can remain when the codec path is left armed.
    recording = false;
    playing = false;

    M5Cardputer.Mic.end();
    audioReady = false;

    M5Cardputer.Speaker.stop();
    delay(12);
    M5Cardputer.Speaker.end();
    delay(20);
}

bool startRecord()
{
    if (!storageReady()) return false;
    if (!begin()) return false;

    if (!SD.exists(AUDIO_DIR)) SD.mkdir(AUDIO_DIR);
    if (SD.exists(TMP_PATH)) SD.remove(TMP_PATH);

    recFile = SD.open(TMP_PATH, FILE_WRITE);
    if (!recFile) return false;

    writeWavHeader(recFile, 0);
    recBytesWritten = 0;
    recStartMs = millis();
    recording = true;
    tmpReady = false;
    level = 0;
    systemLog("REC START");
    return true;
}

bool recordingActive()
{
    return recording;
}

bool updateRecord()
{
    if (!recording || !recFile) return false;

    bool ok = M5Cardputer.Mic.record(recBuf, REC_SAMPLES, WAV_SAMPLE_RATE);
    if (!ok) return false;

    while (M5Cardputer.Mic.isRecording()) {
        delay(1);
    }

    level = calcLevel(recBuf, REC_SAMPLES);
    size_t bytes = REC_SAMPLES * sizeof(int16_t);
    size_t written = recFile.write(reinterpret_cast<const uint8_t*>(recBuf), bytes);
    recBytesWritten += written;
    return written == bytes;
}

bool stopRecord()
{
    if (!recording) return false;

    recording = false;

    if (recFile) {
        writeWavHeader(recFile, recBytesWritten);
        recFile.flush();
        recFile.close();
        tmpReady = recBytesWritten > 0;
    }

    // Release capture path immediately after recording, so speaker playback
    // can reclaim the codec cleanly on Cardputer-Adv.
    M5Cardputer.Mic.end();
    audioReady = false;

    systemLog("REC STOP " + String(recBytesWritten) + "B");
    return tmpReady;
}

void discardRecord()
{
    if (recording) stopRecord();
    if (SD.exists(TMP_PATH)) SD.remove(TMP_PATH);
    tmpReady = false;
    recBytesWritten = 0;
    level = 0;
    systemLog("REC DISCARD");
}

String normalizeWavName(const String& userName)
{
    String n = cleanBaseName(userName);
    String low = n;
    low.toLowerCase();
    if (!low.endsWith(".wav")) n += ".wav";
    return n;
}

bool saveRecordAs(const String& userName, String& outPath)
{
    outPath = "";
    if (!tmpReady || !SD.exists(TMP_PATH)) return false;
    if (!SD.exists(AUDIO_DIR)) SD.mkdir(AUDIO_DIR);

    String name = normalizeWavName(userName);
    String dst = uniquePath(String(AUDIO_DIR) + "/" + name);

    if (SD.exists(dst)) SD.remove(dst);
    bool ok = SD.rename(TMP_PATH, dst);
    if (ok) {
        outPath = dst;
        tmpReady = false;
        systemLog("REC SAVE " + dst);
    }
    return ok;
}

bool tempReady()
{
    return tmpReady;
}

uint32_t recordMillis()
{
    if (!recording && recBytesWritten == 0) return 0;
    if (recording) return millis() - recStartMs;
    return (recBytesWritten / (WAV_SAMPLE_RATE * 2UL)) * 1000UL;
}

uint32_t recordBytes()
{
    return recBytesWritten;
}

int16_t lastLevel()
{
    return level;
}

int16_t readLiveLevel()
{
    if (!begin()) return 0;
    if (playing) return 0;

    bool ok = M5Cardputer.Mic.record(recBuf, REC_SAMPLES, WAV_SAMPLE_RATE);
    if (!ok) return 0;

    while (M5Cardputer.Mic.isRecording()) {
        delay(1);
    }

    level = calcLevel(recBuf, REC_SAMPLES);
    return level;
}

bool playerOpen(const String& path)
{
    playerClose();

    playFile = SD.open(path, FILE_READ);
    if (!playFile) return false;
    if (!parseWavHeader(playFile)) {
        playFile.close();
        return false;
    }

    playPath = path;
    int slash = path.lastIndexOf('/');
    playBaseName = (slash >= 0) ? path.substring(slash + 1) : path;
    playReadBytes = 0;
    playFile.seek(playDataStart);
    return true;
}

void playerClose()
{
    playerStop();
    if (playFile) playFile.close();
    playPath = "";
    playBaseName = "";
    playDataBytes = 0;
    playReadBytes = 0;
}

bool playerStart()
{
    if (!playFile) return false;

    enterPlaybackMode();

    playFile.seek(playDataStart + playReadBytes);
    playing = true;
    systemLog("PLAY START " + playPath + " sr=" + String(playSampleRate) + " ch=" + String(playChannels) + " bytes=" + String(playDataBytes));
    return true;
}

void playerStop()
{
    if (playing) systemLog("PLAY STOP " + playPath);
    playing = false;
    M5Cardputer.Speaker.stop();
    delay(5);
}

void playerUpdate()
{
    if (!playing || !playFile) return;

    if (M5Cardputer.Speaker.isPlaying()) return;

    if (playReadBytes >= playDataBytes) {
        playerStop();
        playReadBytes = 0;
        playFile.seek(playDataStart);
        return;
    }

    uint32_t remaining = playDataBytes - playReadBytes;
    size_t want = PLAY_SAMPLES * sizeof(int16_t);
    if (remaining < want) want = remaining;

    size_t got = playFile.read(reinterpret_cast<uint8_t*>(playBuf), want);
    if (got == 0) {
        playerStop();
        return;
    }

    playReadBytes += got;
    size_t samples = got / sizeof(int16_t);
    M5Cardputer.Speaker.setVolume(playbackVolume);
    M5Cardputer.Speaker.playRaw(playBuf, samples, playSampleRate, playChannels == 2);
}

bool playerPlaying()
{
    return playing;
}

String playerName()
{
    return playBaseName;
}

uint32_t playerPositionMs()
{
    uint32_t bytesPerSec = playSampleRate * playChannels * (playBits / 8);
    if (bytesPerSec == 0) return 0;
    return (playReadBytes * 1000UL) / bytesPerSec;
}

uint32_t playerDurationMs()
{
    uint32_t bytesPerSec = playSampleRate * playChannels * (playBits / 8);
    if (bytesPerSec == 0) return 0;
    return (playDataBytes * 1000UL) / bytesPerSec;
}

void playerSetVolume(uint8_t volume)
{
    playbackVolume = constrain((int)volume, 0, 255);
    M5Cardputer.Speaker.setVolume(playbackVolume);
}

uint8_t playerVolume()
{
    return playbackVolume;
}

} // namespace AudioLab
