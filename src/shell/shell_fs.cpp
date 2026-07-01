#include "shell_internal.h"

void sdGoBack() {
    if (S.sdPath == "/") {
        S.section = ROOT;
        return;
    }

    int last = S.sdPath.lastIndexOf('/');
    S.sdPath = (last <= 0) ? "/" : S.sdPath.substring(0, last);
}

String pathJoin(const String& base, const String& name) {
    return (base == "/") ? "/" + name : base + "/" + name;
}

static String readLimitedLine(File& f, size_t limit)
{
    String out;
    out.reserve(limit < 80 ? limit : 80);
    while (f.available()) {
        char c = (char)f.read();
        if (c == '\n') break;
        if (c == '\r') continue;
        if (out.length() < limit) out += c;
    }
    return out;
}

static bool removeRecursiveDepth(const String& path, uint8_t depth);
static bool copyDirRecursiveDepth(const String& src, const String& dst, uint8_t depth);

bool entryIsDir(const String& path, bool& isDir) {
    File f = SD.open(path);
    if (!f) return false;
    isDir = f.isDirectory();
    f.close();
    return true;
}

static bool removeRecursiveDepth(const String& path, uint8_t depth) {
    if (depth > 12) return false;
    File f = SD.open(path);
    if (!f) return false;

    if (!f.isDirectory()) {
        f.close();
        return SD.remove(path);
    }

    File child = f.openNextFile();
    while (child) {
        String childName = cleanName(String(child.name()));
        String childPath = pathJoin(path, childName);
        bool childIsDir = child.isDirectory();
        child.close();

        bool ok = childIsDir ? removeRecursiveDepth(childPath, depth + 1) : SD.remove(childPath);
        if (!ok) {
            f.close();
            return false;
        }

        child = f.openNextFile();
    }

    f.close();
    return SD.rmdir(path);
}

bool removeRecursive(const String& path) {
    return removeRecursiveDepth(path, 0);
}

bool copyFileData(const String& src, const String& dst) {
    File in = SD.open(src, FILE_READ);
    if (!in || in.isDirectory()) {
        if (in) in.close();
        return false;
    }

    if (SD.exists(dst)) SD.remove(dst);
    File out = SD.open(dst, FILE_WRITE);
    if (!out) {
        in.close();
        return false;
    }

    uint8_t buf[512];
    bool ok = true;
    while (in.available()) {
        size_t n = in.read(buf, sizeof(buf));
        if (n == 0) break;
        if (out.write(buf, n) != n) {
            ok = false;
            break;
        }
        delay(0);
    }

    out.close();
    in.close();
    return ok;
}

static bool copyDirRecursiveDepth(const String& src, const String& dst, uint8_t depth) {
    if (depth > 12) return false;
    File dir = SD.open(src);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return false;
    }

    if (!SD.exists(dst) && !SD.mkdir(dst)) {
        dir.close();
        return false;
    }

    File child = dir.openNextFile();
    while (child) {
        String childName = cleanName(String(child.name()));
        String srcChild = pathJoin(src, childName);
        String dstChild = pathJoin(dst, childName);
        bool childIsDir = child.isDirectory();
        child.close();

        bool ok = childIsDir ? copyDirRecursiveDepth(srcChild, dstChild, depth + 1) : copyFileData(srcChild, dstChild);
        if (!ok) {
            dir.close();
            return false;
        }

        child = dir.openNextFile();
        delay(0);
    }

    dir.close();
    return true;
}

bool copyDirRecursive(const String& src, const String& dst) {
    return copyDirRecursiveDepth(src, dst, 0);
}

String conflictFreePath(const String& dir, const String& name, bool isDir) {
    String preferred = pathJoin(dir, name);
    if (!SD.exists(preferred)) return preferred;

    if (isDir) {
        for (int i = 1; i < 100; ++i) {
            String suffix = "_copy";
            if (i > 1) suffix += String(i);
            String p = pathJoin(dir, name + suffix);
            if (!SD.exists(p)) return p;
        }
        return pathJoin(dir, name + "_copy_new");
    }

    int dot = name.lastIndexOf('.');
    String base = (dot > 0) ? name.substring(0, dot) : name;
    String ext  = (dot > 0) ? name.substring(dot) : "";

    for (int i = 1; i < 100; ++i) {
        String suffix = "_copy";
        if (i > 1) suffix += String(i);
        String p = pathJoin(dir, base + suffix + ext);
        if (!SD.exists(p)) return p;
    }

    return pathJoin(dir, base + "_copy_new" + ext);
}

void newFile(String name) {
    if (S.section != SD_SECTION) {
        addHistory("NEW ONLY IN SD", RED);
        return;
    }

    name.trim();
    if (!isSafeName(name) || !looksLikeFileName(name)) {
        addHistory("USE new <file.ext>", RED);
        soundError();
        return;
    }

    String path;
    if (!buildSdPath(name, path)) {
        addHistory("INVALID NAME", RED);
        soundError();
        return;
    }

    if (SD.exists(path)) {
        addHistory("EXISTS " + upperCopy(name), YELLOW);
        soundError();
        return;
    }

    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        addHistory("CREATE FAIL", RED);
        soundError();
        return;
    }

    f.close();
    addHistory("CREATED " + upperCopy(name), GREEN);
    systemLog("FILE CREATE " + path);
    soundSuccess();
}

void delEntry(String name) {
    if (S.section != SD_SECTION) {
        addHistory("DEL ONLY IN SD", RED);
        return;
    }

    name.trim();
    String path;
    if (!buildSdPath(name, path)) {
        addHistory("INVALID NAME", RED);
        soundError();
        return;
    }

    if (!SD.exists(path)) {
        addHistory("NOT FOUND " + upperCopy(name), RED);
        soundError();
        return;
    }

    bool isDir = false;
    if (!entryIsDir(path, isDir)) {
        addHistory("OPEN FAIL", RED);
        soundError();
        return;
    }

    bool ok = isDir ? removeRecursive(path) : SD.remove(path);
    if (ok) {
        addHistory(String(isDir ? "DIR DELETED " : "DELETED ") + upperCopy(name), GREEN);
        systemLog(String(isDir ? "DIR DELETE " : "FILE DELETE ") + path);
        soundSuccess();
    } else {
        addHistory("DEL FAIL", RED);
        soundError();
    }
}

void copyEntry(String name) {
    if (S.section != SD_SECTION) {
        addHistory("COPY ONLY IN SD", RED);
        return;
    }

    name.trim();
    String path;
    if (!buildSdPath(name, path)) {
        addHistory("INVALID NAME", RED);
        soundError();
        return;
    }

    if (!SD.exists(path)) {
        addHistory("NOT FOUND " + upperCopy(name), RED);
        soundError();
        return;
    }

    bool isDir = false;
    if (!entryIsDir(path, isDir)) {
        addHistory("OPEN FAIL", RED);
        soundError();
        return;
    }

    S.clipActive = true;
    S.clipPath = path;
    S.clipName = cleanName(path);
    S.clipIsDir = isDir;

    addHistory("COPIED " + upperCopy(S.clipName), GREEN);
    systemLog("COPY " + path);
    soundSuccess();
}

void pasteEntry() {
    if (S.section != SD_SECTION) {
        addHistory("PASTE ONLY IN SD", RED);
        return;
    }

    if (!S.clipActive || S.clipPath.length() == 0) {
        addHistory("CLIPBOARD EMPTY", YELLOW);
        soundError();
        return;
    }

    if (!SD.exists(S.clipPath)) {
        addHistory("SOURCE LOST", RED);
        S.clipActive = false;
        soundError();
        return;
    }

    String dst = conflictFreePath(S.sdPath, S.clipName, S.clipIsDir);

    if (dst == S.clipPath || dst.startsWith(S.clipPath + "/")) {
        addHistory("BAD DESTINATION", RED);
        soundError();
        return;
    }

    bool ok = S.clipIsDir ? copyDirRecursive(S.clipPath, dst) : copyFileData(S.clipPath, dst);
    if (ok) {
        addHistory("PASTED " + upperCopy(cleanName(dst)), GREEN);
        systemLog("PASTE " + S.clipPath + " -> " + dst);
        soundSuccess();
    } else {
        addHistory("PASTE FAIL", RED);
        soundError();
    }
}

void catFile(String name) {
    if (S.section != SD_SECTION) {
        addHistory("CAT ONLY IN SD", RED);
        return;
    }

    name.trim();

    String path;
    if (!buildSdPath(name, path)) {
        addHistory("INVALID NAME", RED);
        soundError();
        return;
    }

    if (isAudioFile(name)) {
        addHistory("AUDIO FILE: USE open", YELLOW);
        return;
    }

    File f = SD.open(path, FILE_READ);
    if (!f || f.isDirectory()) {
        addHistory("OPEN FAIL", RED);
        if (f) f.close();
        soundError();
        return;
    }

    addHistory("");
    addHistory("FILE " + upperCopy(name), CYAN);

    int cnt = 0;
    while (f.available() && cnt < Limit::CAT_LINES) {
        String ln = readLimitedLine(f, Limit::CAT_LEN);
        ln.replace("\r", "");
        addHistory(ln, WHITE);
        cnt++;
    }

    if (f.available()) addHistory("...MORE", DARKGREY);
    f.close();
}

void mkDir(String name) {
    if (S.section != SD_SECTION) {
        addHistory("MKDIR ONLY IN SD", RED);
        return;
    }

    name.trim();
    if (!isSafeName(name) || name.indexOf('.') >= 0) {
        addHistory("INVALID NAME", RED);
        soundError();
        return;
    }

    String path;
    if (!buildSdPath(name, path)) {
        addHistory("INVALID PATH", RED);
        soundError();
        return;
    }

    if (SD.exists(path)) {
        addHistory("EXISTS " + upperCopy(name), YELLOW);
        soundError();
        return;
    }

    if (SD.mkdir(path)) {
        addHistory("CREATED " + upperCopy(name), GREEN);
        systemLog("DIR CREATE " + path);
        soundSuccess();
    } else {
        addHistory("MKDIR FAIL", RED);
        soundError();
    }
}

void initLayout() {
    if (!storageReady()) {
        addHistory("SD NOT READY", RED);
        soundError();
        return;
    }

    if (storageCreateLayout()) {
        addHistory("LAYOUT CREATED", GREEN);
        addHistory("/scripts OK", GREEN);
        addHistory("/apps OK", GREEN);
        addHistory("/data OK", GREEN);
        addHistory("/data/tmp OK", GREEN);
        addHistory("/data/idchat OK", GREEN);
        addHistory("/audio OK", GREEN);
        addHistory("/backup OK", GREEN);
        addHistory("/docs OK", GREEN);
        systemLog("LAYOUT INIT");
        soundSuccess();
    } else {
        addHistory("LAYOUT FAIL", RED);
        soundError();
    }
}

// ───────────────────────────────────────────────────────────────────
