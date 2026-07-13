#pragma once
#include <cstdint>
#include "input/InputBroker.h"
#include "SnakeGame.h"
#include "FlappyBird.h"

class OLEDDisplay;

/**
 * @brief Top-level facade for in-OLED games.
 *
 * Owns a SnakeGame and a FlappyBird instance. Only one is active at a time.
 * State machine (per module):
 *   Idle    - nothing on screen, list shown
 *   Playing - active game is alive
 *   Paused  - active game is frozen, banner shown
 *
 * Switching games: call quit() then startSnake()/startFlappy().
 *
 * Single global instance exposed via `gamesModule`.
 */
class GamesModule
{
  public:
    enum State { Idle, Playing, Paused };
    enum GameType { None, Snake, Flappy };

    GamesModule();

    /// Boot Snake from its current reset state. No-op if a game is active.
    void startSnake();

    /// Boot Flappy Bird from its current reset state. No-op if a game is active.
    void startFlappy();

    /// One-way: Playing -> Paused. No-op otherwise.
    void pause();

    /// One-way: Paused -> Playing. Preserves game state.
    void resume();

    /// Toggle Playing/Paused.
    void togglePause();

    /// Force-stop and forget state.
    void quit();

    /// Restart the current game with fresh seed.
    void restart();

    bool isActive() const { return state != Idle; }
    bool isPlaying() const { return state == Playing; }
    bool isPaused() const { return state == Paused; }
    GameType getActiveGame() const { return activeGame; }
    State getState() const { return state; }

    /// Score of the active game (or 0 if Idle).
    int getScore() const;

    bool handleInput(input_broker_event ev);
    void tick(uint32_t nowMs);
    void draw(OLEDDisplay *display, int16_t x, int16_t y);

  private:
    SnakeGame snake;
    FlappyBird flappy;
    GameType activeGame = None;
    State state = Idle;
    uint32_t lastTickAt = 0;
    // Snapshot of the active game's State enum value (Idle=0 Playing=1 Paused=2 GameOver=3).
    // Both SnakeGame and FlappyBird share the same enum values, so we can stash either.
    uint8_t lastGameState = 0;

    void setState(State s) { state = s; }
};

extern GamesModule *gamesModule;
