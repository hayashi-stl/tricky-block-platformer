#include "GameManager.h"

GameManager::GameManager()
{
    currLevel = 0xff;

}

void GameManager::changeState(uint8_t newState, uint8_t newLevel, bool firstTime = false)
{
    if(!firstTime)
        currState->final();

    if(newState == GameState::LEVEL)
        currState.reset(new LevelState(newLevel));
    else
        currState.reset(gameStates[newState]());

    update();
}

void GameManager::update()
{
    array<uint8_t, 2> newStateLevel = currState->update();
    if(newStateLevel[0] != GameState::DONT_CHANGE || newStateLevel[1] != 0xff)
        changeState(newStateLevel[0], newStateLevel[1]);
}

GameState* GameManager::getState()
{
    return currState.get();
}

GameManager gm;
