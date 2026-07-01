#pragma once
#include <Arduino.h>
#include <M5Cardputer.h>

// idDOS keyboard compatibility layer for M5Cardputer / Cardputer-Adv.
// IMPORTANT: do not invent a PC keyboard layout here.
// M5Cardputer KeysState v1.1.x exposes fn/shift/ctrl/opt/alt/del/enter/space/word/hid_keys,
// but not esc/up/down/left/right/backspace fields.
// Physical Cardputer navigation labels are:
//   ESC: top-left ` / ~ key
//   UP:    FN + ;
//   DOWN:  FN + .
//   LEFT:  FN + ,
//   RIGHT: FN + /
// We also accept HID arrow codes and old FN+WASD as fallback only.

inline bool keyWordHas(const Keyboard_Class::KeysState& ks, char c)
{
    for (auto v : ks.word) {
        if ((char)v == c) return true;
    }
    return false;
}

inline bool keyHidHas(const Keyboard_Class::KeysState& ks, uint8_t code)
{
    for (auto v : ks.hid_keys) {
        if (v == code) return true;
    }
    return false;
}

inline bool keyBackspace(const Keyboard_Class::KeysState& ks)
{
    // Library calls the physical delete/backspace key "del".
    return ks.del || keyHidHas(ks, 0x2a) || keyWordHas(ks, '\b') || keyWordHas(ks, 127);
}

inline bool keyEsc(const Keyboard_Class::KeysState& ks)
{
    // Physical top-left key is printed as ESC / ~ but library reports it as ` or ~.
    // Treat it as ESC in idDOS UI; backtick text input is not important for the shell UX.
    return keyWordHas(ks, 27) || keyWordHas(ks, '`') || keyWordHas(ks, '~') || keyHidHas(ks, 0x29);
}

inline bool keyUp(const Keyboard_Class::KeysState& ks)
{
    return keyHidHas(ks, 0x52) ||
           (ks.fn && (keyWordHas(ks, ';') || keyWordHas(ks, ':') ||
                      keyWordHas(ks, 'w') || keyWordHas(ks, 'W')));
}

inline bool keyDown(const Keyboard_Class::KeysState& ks)
{
    return keyHidHas(ks, 0x51) ||
           (ks.fn && (keyWordHas(ks, '.') || keyWordHas(ks, '>') ||
                      keyWordHas(ks, 's') || keyWordHas(ks, 'S')));
}

inline bool keyLeft(const Keyboard_Class::KeysState& ks)
{
    return keyHidHas(ks, 0x50) ||
           (ks.fn && (keyWordHas(ks, ',') || keyWordHas(ks, '<') ||
                      keyWordHas(ks, 'a') || keyWordHas(ks, 'A')));
}

inline bool keyRight(const Keyboard_Class::KeysState& ks)
{
    return keyHidHas(ks, 0x4f) ||
           (ks.fn && (keyWordHas(ks, '/') || keyWordHas(ks, '?') ||
                      keyWordHas(ks, 'd') || keyWordHas(ks, 'D')));
}

inline bool keyPlus(const Keyboard_Class::KeysState& ks)
{
    // Physical + is shift/Aa + = on Cardputer.
    return keyWordHas(ks, '+') || keyWordHas(ks, '=');
}

inline bool keyMinus(const Keyboard_Class::KeysState& ks)
{
    return keyWordHas(ks, '-') || keyWordHas(ks, '_');
}
