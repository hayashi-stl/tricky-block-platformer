#include "Globals.h"

bool keyStates[256] = {0};
bool keySStates[256] = {0};
float mousePos[2] = {0.0f};
bool mouseStates[3] = {0};

bool bufKeyStates[256] = {0};
bool bufKeySStates[256] = {0};
float bufMousePos[2] = {0.0f};
bool bufMouseStates[3] = {0};

uint8_t keyMods = 0;
uint8_t bufKeyMods = 0;
