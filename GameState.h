#ifndef GAMESTATE_H_INCLUDED
#define GAMESTATE_H_INCLUDED
#include <array>
#include "mesh.h"

using namespace std;

struct Option
{
    string option;
    list<shared_ptr<GeneralObj>>::iterator vertices;
    float xPos;
    float yPos;
    float left;
    float right;
    float top;
    float bottom;
    bool isInsideRect(float x, float y)
    {
        return x >= left && x < right && y >= bottom && y < top;
    }
};
struct Description
{
    string desc;
    float xPos;
    float yPos;
};

struct Message
{
    string name;
    list<shared_ptr<GeneralObj>>::iterator vertices;
    float xPos;
    float yPos;
    bool active;
    const int16_t maxTime = 300;
    int16_t timer;
};

class GameState
{

public:
    enum states {MENU, LEVEL, LEVEL_EDITOR, LEVEL_SELECT};
    static const uint8_t DONT_CHANGE = 0xff;

    static const uint8_t numLevels = 20;
    static const uint8_t startLevel = 1;

    GameState(){}

    //returns which state to change to
    //and level if necessary
    //is 0xffff if state should not change
    virtual array<uint8_t, 2> update() = 0;
    virtual void final() = 0;

};
#endif // GAMESTATE_H_INCLUDED
