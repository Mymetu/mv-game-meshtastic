#pragma once
#include <cstdint>

class OLEDDisplay;

/**
 * @brief Minimal Flappy Bird for SSD1306 128x64 OLED.
 *
 * Layout (matches SnakeGame):
 *   Game area : 25 cols x 16 rows of 4 px logic, but rendered at 1 px per cell
 *               on the right 100 px of the display (left 28 px is the score
 *               panel drawn by GamesModule).
 *
 * State machine (matches SnakeGame):
 *   Idle     - freshly constructed or just reset
 *   Playing  - active; tick advances physics
 *   Paused   - tick ignored, render still shows field
 *   GameOver - bird collided; SELECT restarts
 */
class FlappyBird
{
  public:
    static constexpr int W = 100; // game-area width in px (after 28-px score panel)
    static constexpr int H = 64;  // game-area height in px
    static constexpr int BIRD_W = 4;
    static constexpr int BIRD_H = 4;
    static constexpr int PIPE_W = 4;       // pipe thickness
    static constexpr int PIPE_GAP = 28;    // vertical opening per pipe (px)
    static constexpr int PIPE_SPACING = 36; // horizontal distance between pipe spawns (px)
    static constexpr int MAX_PIPES = 4;    // ring buffer; PIPE_SPACING should keep ≤ 3 active

    enum State { Idle, Playing, Paused, GameOver };

    FlappyBird();

    /// Reset to the initial idle state, spawn first pipe, ready to start().
    void reset();

    /// Begin (or restart) playing.
    void start();

    /// Suspend movement (render still draws).
    void pause();

    /// Resume from Paused.
    void resume();

    /// Mark game over (bird collided).
    void gameOver();

    /// Apply an upward impulse (tap). Ignored outside Playing.
    void flap();

    /// Advance one physics frame. Caller paces this via stepIntervalMs().
    void step();

    /// Per-frame cadence in ms. Faster than snake; flappy is reaction-based.
    uint32_t stepIntervalMs() const { return stepMs; }

    State getState() const { return state; }
    int getScore() const { return score; }
    int getBestScore() const { return bestScore; }

    /// Paint the play field. Caller is responsible for offsetting past the score panel.
    void draw(OLEDDisplay *display, int16_t x, int16_t y);

  private:
    struct Pipe
    {
        int8_t x;        // top-left x; -1 means slot is empty
        uint8_t gapY;    // top of gap (0..H - PIPE_GAP)
        bool scored;     // has this pipe been counted toward score?
    };

    // Bird physics (use fixed-point to keep things simple).
    int16_t birdX;       // top-left x
    int16_t birdY;       // top-left y
    int8_t velocity;     // y-velocity; +down, -up (units: px per frame)

    // Pipe ring buffer.
    Pipe pipes[MAX_PIPES];
    uint8_t pipeHead;    // next slot to recycle when spawning

    int score;
    int bestScore;
    uint32_t stepMs;
    uint8_t spawnTimer; // frames remaining until next pipe spawn
    bool started;       // false until first flap; pauses physics so the player
                        // can read the screen before the bird starts dropping.
    State state;

    void spawnPipe();
    bool birdHits(uint8_t bx, uint8_t by, uint8_t bw, uint8_t bh, int8_t px, uint8_t py, uint8_t pw,
                  uint8_t gapY) const;
};
