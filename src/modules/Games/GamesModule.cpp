#include "GamesModule.h"
#include "SnakeGame.h"
#include "FlappyBird.h"
#include "graphics/Screen.h"
#include "graphics/ScreenFonts.h"
#include "graphics/draw/MenuHandler.h"
#include <OLEDDisplay.h>
#include <Arduino.h>

#ifdef ENABLE_GAMES_FRAME

GamesModule::GamesModule() : snake(), flappy(), activeGame(None), state(Idle), lastTickAt(0), lastGameState(0) {}

int GamesModule::getScore() const
{
    switch (activeGame) {
    case Snake:
        return snake.getScore();
    case Flappy:
        return flappy.getScore();
    default:
        return 0;
    }
}

void GamesModule::startSnake()
{
    if (state != Idle)
        return;
    randomSeed(millis() ^ micros());
    snake.start();
    state = Playing;
    activeGame = Snake;
    lastTickAt = millis();
    lastGameState = (uint8_t)snake.getState();
}

void GamesModule::startFlappy()
{
    if (state != Idle)
        return;
    randomSeed(millis() ^ micros());
    flappy.start();
    state = Playing;
    activeGame = Flappy;
    lastTickAt = millis();
    lastGameState = (uint8_t)flappy.getState();
}

void GamesModule::pause()
{
    if (state != Playing)
        return;
    if (activeGame == Snake)
        snake.pause();
    else if (activeGame == Flappy)
        flappy.pause();
    state = Paused;
}

void GamesModule::resume()
{
    if (state != Paused)
        return;
    if (activeGame == Snake)
        snake.resume();
    else if (activeGame == Flappy)
        flappy.resume();
    state = Playing;
    // Re-anchor tick so we don't instantly step a beat after resume.
    lastTickAt = millis();
}

void GamesModule::togglePause()
{
    if (state == Playing) {
        pause();
    } else if (state == Paused) {
        resume();
    }
}

void GamesModule::quit()
{
    if (activeGame == Snake) {
        snake.reset();
        snake.pause(); // sets state to Idle via reset(); redundant safe
    } else if (activeGame == Flappy) {
        flappy.reset();
        flappy.pause();
    }
    state = Idle;
    activeGame = None;
    lastGameState = 0;
}

void GamesModule::restart()
{
    randomSeed(millis() ^ micros());
    if (activeGame == Snake) {
        snake.start();
    } else if (activeGame == Flappy) {
        flappy.start();
    }
    state = Playing;
    lastTickAt = millis();
    lastGameState = (activeGame == Snake) ? (uint8_t)snake.getState() : (uint8_t)flappy.getState();
}

bool GamesModule::handleInput(input_broker_event ev)
{
    if (state == Idle)
        return false;

    // Page-lock: USER_PRESS / BACK would otherwise flip frames out of the game page.
    if (ev == INPUT_BROKER_USER_PRESS || ev == INPUT_BROKER_BACK) {
        return true;
    }

    if (state == Paused) {
        if (ev == INPUT_BROKER_CANCEL_LONG) {
            togglePause();
            return true;
        }
        if (ev == INPUT_BROKER_SELECT) {
            // Quit: return to Idle so the game list shows again.
            quit();
            return true;
        }
        if (ev == INPUT_BROKER_UP || ev == INPUT_BROKER_DOWN ||
            ev == INPUT_BROKER_LEFT || ev == INPUT_BROKER_RIGHT) {
            return true; // consume, do nothing
        }
        return false;
    }

    // Playing
    if (activeGame == Snake) {
        switch (ev) {
        case INPUT_BROKER_UP:
            if (snake.getState() == SnakeGame::Playing) snake.setDir(0, -1);
            return true;
        case INPUT_BROKER_DOWN:
            if (snake.getState() == SnakeGame::Playing) snake.setDir(0, +1);
            return true;
        case INPUT_BROKER_LEFT:
            if (snake.getState() == SnakeGame::Playing) snake.setDir(-1, 0);
            return true;
        case INPUT_BROKER_RIGHT:
            if (snake.getState() == SnakeGame::Playing) snake.setDir(+1, 0);
            return true;
        case INPUT_BROKER_CANCEL_LONG:
            return false; // handled in Screen.cpp
        case INPUT_BROKER_SELECT:
            if (snake.getState() == SnakeGame::GameOver) {
                restart();
            }
            return true;
        default:
            return false;
        }
    }

    if (activeGame == Flappy) {
        switch (ev) {
        case INPUT_BROKER_UP:
        case INPUT_BROKER_SELECT:
            // Both UP and SELECT flap. SELECT has a fail-safe restart role during
            // GameOver but is also a comfortable tap location mid-play.
            if (flappy.getState() == FlappyBird::GameOver) {
                restart();
            } else {
                flappy.flap();
            }
            return true;
        // Direction keys are swallowed so LEFT/RIGHT/DOWN can't flip frames out
        // of the dedicated game page while the user is playing.
        case INPUT_BROKER_DOWN:
        case INPUT_BROKER_LEFT:
        case INPUT_BROKER_RIGHT:
            return true;
        case INPUT_BROKER_CANCEL_LONG:
            return false; // handled in Screen.cpp
        default:
            return false;
        }
    }

    return false;
}

void GamesModule::tick(uint32_t nowMs)
{
    if (state != Playing)
        return;

    uint32_t intervalMs = 0;
    if (activeGame == Snake) {
        intervalMs = snake.stepIntervalMs();
    } else if (activeGame == Flappy) {
        intervalMs = flappy.stepIntervalMs();
    } else {
        return;
    }

    if ((nowMs - lastTickAt) >= intervalMs) {
        if (activeGame == Snake)
            snake.step();
        else if (activeGame == Flappy)
            flappy.step();
        lastTickAt = nowMs;

        // Detect Playing -> GameOver transition ONCE and pop the banner.
        uint8_t after = 0;
        if (activeGame == Snake)
            after = (uint8_t)snake.getState();
        else if (activeGame == Flappy)
            after = (uint8_t)flappy.getState();

        // GameOver enum value is 3 for both classes.
        if (lastGameState != 3 && after == 3) {
            graphics::menuHandler::menuQueue = graphics::menuHandler::GameOverBanner;
            if (screen) screen->runNow();
        }
        lastGameState = after;
    }
}

void GamesModule::draw(OLEDDisplay *display, int16_t x, int16_t y)
{
    if (state == Idle)
        return;

    // Score panel is 28 px wide; game area is 100 px wide (matches Snake layout).
    const int panelW = 28;

    // Game field, anchored to the right of the score panel. Each game draws
    // its own bounds (clear + border + content) inside (x+panelW, y).
    if (activeGame == Snake)
        snake.draw(display, x + panelW, y);
    else if (activeGame == Flappy)
        flappy.draw(display, x + panelW, y);

    // Score panel: "PTS" header + active game's score.
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", getScore());

    display->setTextAlignment(TEXT_ALIGN_CENTER);
    display->setFont(FONT_SMALL);
    const int panelCx = x + panelW / 2;
    display->drawString(panelCx, y + 2, "PTS");
    display->drawString(panelCx, y + 18, buf);

    // Subtle vertical divider so the panel reads as a separate column.
    display->drawLine(x + panelW - 1, y, x + panelW - 1, y + 64);

    display->setTextAlignment(TEXT_ALIGN_LEFT);
}

#endif // ENABLE_GAMES_FRAME

#ifndef ENABLE_GAMES_FRAME
// Stub implementations for builds without ENABLE_GAMES_FRAME.
// gamesModule stays nullptr (see main.cpp), so these are never actually called,
// but the symbols must exist so Screen.cpp / MenuHandler.cpp / DebugRenderer.cpp
// can link against the class without extra #ifdefs.
GamesModule::GamesModule() : snake(), flappy(), activeGame(None), state(Idle), lastTickAt(0), lastGameState(0) {}

int GamesModule::getScore() const
{
    return 0;
}

void GamesModule::startSnake() {}

void GamesModule::startFlappy() {}

void GamesModule::pause() {}

void GamesModule::resume() {}

void GamesModule::togglePause() {}

void GamesModule::quit() {}

void GamesModule::restart() {}

bool GamesModule::handleInput(input_broker_event ev)
{
    return false;
}

void GamesModule::tick(uint32_t nowMs) {}

void GamesModule::draw(OLEDDisplay *display, int16_t x, int16_t y) {}
#endif // !ENABLE_GAMES_FRAME

// Global pointer is defined unconditionally so external code (Screen.cpp /
// DebugRenderer.cpp / MenuHandler.cpp) can always reference it without #ifdefs.
// The pointer simply stays null in builds where the games frame is disabled.
GamesModule *gamesModule = nullptr;
