#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED
#include <string>
#include <memory>

using namespace std;

class Mesh;

const int numAnimFiles = 2;
enum AnimFiles {BONE, MATERIAL};

extern bool keyStates[256];
extern bool keySStates[256];
extern float mousePos[2];
extern bool mouseStates[3];

extern bool bufKeyStates[256];
extern bool bufKeySStates[256];
extern float bufMousePos[2];
extern bool bufMouseStates[3];

extern uint8_t keyMods;
extern uint8_t bufKeyMods;

//for dictating animation files
const string animExten[] = {"b", "m"};

//constants for animation
enum AnimType {LOCATION, ROTATION, SCALE};
enum AnimMatType {DIFFUSE, DIFFUSE_INT, SPECULAR, SPECULAR_INT, HARDNESS};
enum Interpolation {LINEAR, CONSTANT, SINE, QUAD, CUBIC, QUART, QUINT, EXPO, CIRC};
enum Easing {EASE_LEFT, EASE_RIGHT, EASE_BOTH};

const float tau = 6.283185307;



#endif // CONSTANTS_H_INCLUDED
