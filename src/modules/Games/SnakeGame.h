#pragma once
#include <cstdint>

class OLEDDisplay;

/**
 * @brief Minimal Snake game for SSD1306 128x64 OLED.
 *
 * Grid: 16 columns x 8 rows, 4 px per cell, drawn 64x32 in screen center.
 *
 * State machine:
 *   Idle     - freshly constructed or just reset
 *   Playing  - active; tick advances the snake
 *   Paused   - tick is ignored, render still shows grid
 *   GameOver - snake collided; P0 (SELECT) restarts
 */
class SnakeGame
{
  public:
    static constexpr int W = 25; // game-area cols (4 px each) — leaves a 28-px score panel on the left
    static constexpr int H = 16; // game-area rows (4 px each)

    enum State { Idle, Playing, Paused, GameOver };

    SnakeGame();

    /// Reset to the initial 3-cell snake, spawn first food.
    void reset();

    /// Begin (or restart) playing.
    void start();

    /// Suspend movement (render still draws).
    void pause();

    /// Resume from Paused.
    void resume();

    /// Mark game over (snake collided). Caller decides whether to restart.
    void gameOver();

    /// Set the next direction. dx,dy in {-1,0,+1}, exactly one is non-zero.
    /// Reverse of current direction is ignored (forbids instant self-bite).
    void setDir(int dx, int dy);

    /// Advance one cell based on current direction; check collisions; eat/spawn food.
    void step();

    /// Returns the per-step cadence in ms (decreases as score climbs).
    uint32_t stepIntervalMs() const { return stepMs; }

    State getState() const { return state; }
    int getScore() const { return score; }
    int getLength() const { return length; }
    int getDirX() const { return dirX; }
    int getDirY() const { return dirY; }

    /// Paint the play field into the display. Caller has already chosen x,y origin.
    void draw(OLEDDisplay *display, int16_t x, int16_t y);

  private:
    struct Cell
    {
        uint8_t x, y;
    };

    // Snake body is a fixed ring buffer; we never grow past W*H.
    Cell body[W * H];
    uint8_t length; // current body length (head at index 0)
    int8_t dirX, dirY;
    uint8_t foodX, foodY;
    State state;
    int score;
    uint32_t stepMs;

    bool isBodyAt(uint8_t x, uint8_t y) const;
    void placeFood();
    void speedUp();
};
