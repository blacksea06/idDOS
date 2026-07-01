#pragma once
#include <Arduino.h>

void configLoad();
void configSave();

String configUsername();
String configPassword();          // deprecated: never returns stored password in v2
String configPasswordMasked();

void configSetUsername(const String& v);
void configSetPassword(const String& v);
void configClearPassword();

bool configHasPassword();
bool configCheckPassword(const String& input);
