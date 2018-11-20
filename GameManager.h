#ifndef GAMEMANAGER_H_INCLUDED
#define GAMEMANAGER_H_INCLUDED
#include <memory>
#include <array>
#include "GameState.h"
#include "GameStates/MenuState.h"
#include "GameStates/LevelState.h"
#include "GameStates/LevelEditor.h"
#include "GameStates/LevelSelect.h"

using namespace std;

typedef GameState* (*GSMaker)();

template <typename T>
GameState* gsMake()
{
    return new T;
}

class GameManager
{
    unique_ptr<GameState> currState;
    uint8_t currLevel;

public:
    GameManager();

    void changeState(uint8_t, uint8_t, bool);
    void update();
    GameState* getState();

private:
    GSMaker gameStates[4] = {gsMake<MenuState>,
                             gsMake<LevelState>,
                             gsMake<LevelEditor>,
                             gsMake<LevelSelect>};

};

extern GameManager gm;

#endif // GAMEMANAGER_H_INCLUDED
