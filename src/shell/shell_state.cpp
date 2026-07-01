#include "shell_internal.h"

ShellState S;
bool editorMode = false;
bool editorSaveMode = false;
int editorSaveSelection = 0;
std::vector<String> editorLines;
String editorPath = "";
String editorName = "";
int editorScrollY = 0;
int editorScrollX = 0;
int editorCursorY = 0;
int editorCursorX = 0;
int audioWave[120];
int audioWavePos = 0;
uint32_t audioLastWaveMs = 0;
bool lockRequested = false;
