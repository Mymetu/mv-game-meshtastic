#include "SnakeGame.h"
#include "DebugConfiguration.h"
#include "graphics/ScreenFonts.h"
#include "graphics/UiStrings.h"
#include <OLEDDisplay.h>
#include <Arduino.h>

#ifdef ENABLE_GAMES_FRAME

SnakeGame::SnakeGame()
    : length(3), dirX(1), dirY(0), foodX(0), foodY(0), state(Idle), score(0), stepMs(180)
{
    reset();
}

void SnakeGame::reset()
{
    // Initial snake: 3 cells along the middle row going right, head at col W/2
    // (center). This leaves the snake roughly equidistant from every wall so a
    // single mis-press doesn't immediately drive it into a corner.
    body[0] = {(uint8_t)(W / 2),        (uint8_t)(H / 2)};
    body[1] = {(uint8_t)(W / 2 - 1),    (uint8_t)(H / 2)};
    body[2] = {(uint8_t)(W / 2 - 2),    (uint8_t)(H / 2)};
    length = 3;
    dirX = 1;
    dirY = 0;
    score = 0;
    stepMs = 180;
    state = Idle;
    placeFood();
}

void SnakeGame::start()
{
    reset();
    state = Playing;
}

void SnakeGame::pause()
{
    if (state == Playing) {
        state = Paused;
    } else if (state == Paused) {
        state = Playing;
    }
}

void SnakeGame::resume()
{
    if (state == Paused) {
        state = Playing;
    }
}

void SnakeGame::gameOver()
{
    state = GameOver;
}

void SnakeGame::setDir(int dx, int dy)
{
    // Only honor direction changes while the game is actively running.
    // (Old code allowed GameOver/Idle too — that was redundant, since reset()
    // already paints the initial direction.)
    if (state != Playing)
        return;
    if (dx == 0 && dy == 0)
        return;
    // Forbid 180-degree reversal.
    if (dx == -dirX && dy == -dirY)
        return;
    // Forbid non-axis moves.
    if (dx != 0 && dy != 0)
        return;
    dirX = dx;
    dirY = dy;
}

void SnakeGame::step()
{
    if (state != Playing)
        return;

    int nx = (int)body[0].x + dirX;
    int ny = (int)body[0].y + dirY;

    LOG_INFO("Snake step: dir=(%d,%d) head=(%d,%d) -> next=(%d,%d) food=(%d,%d) len=%d",
             dirX, dirY, body[0].x, body[0].y, nx, ny, foodX, foodY, length);

    // Wall collision.
    if (nx < 0 || nx >= W || ny < 0 || ny >= H) {
        gameOver();
        return;
    }

    bool ate = (nx == foodX && ny == foodY);

    // Self-collision rule: a normal move lets the tail slip out of the way, so
    // landing on the current tail cell is legal. Eating makes the tail stay,
    // so it must be excluded. This check is what fixes "eat one → game over".
    {
        int checkLimit = ate ? length : (length - 1);
        for (int i = 1; i < checkLimit; ++i) {
            if ((int)body[i].x == nx && (int)body[i].y == ny) {
                gameOver();
                return;
            }
        }
    }

    // Shift body right by one (tail drops off when not eating; tail sticks when eating).
    for (int i = length; i > 0; --i) {
        body[i] = body[i - 1];
    }
    body[0] = {(uint8_t)nx, (uint8_t)ny};

    if (ate) {
        score += 10;
        speedUp();
        // Grow: keep the tail (which we just duplicated) by extending the active length.
        // NB: do NOT cast (W*H) to uint8_t — 512 truncates to 0 and the else-branch
        // would gameOver() the very first time we ate anything. Compare in int.
        if (length < (W * H)) {
            LOG_INFO("Snake ATE: len %d -> %d, new food at (%d,%d)",
                     length, length + 1, foodX, foodY);
            length++;
            placeFood();
        } else {
            // Filled the board — that counts as a win.
            LOG_INFO("Snake WIN: full board");
            gameOver();
            return;
        }
    }
}

void SnakeGame::draw(OLEDDisplay *display, int16_t x, int16_t y)
{
    // Full-screen play field: 25 cols x 16 rows of 4-px cells, anchors at (x,y).
    const int cellPx = 4;
    const int fieldPxW = W * cellPx; // 100
    const int fieldPxH = H * cellPx; // 64 (grid bottom; border lifted 2 px below)

    // Defensive clear: OLEDDisplayUi clears before invoking frame callbacks, but
    // re-clearing here makes the draw self-contained.
    display->clear();

    // 1-pixel border around the play field; lift the bottom edge by 2 px so the
    // outline doesn't sit flush against the OLED bottom — breathing room.
    display->drawRect(x, y, fieldPxW, fieldPxH - 2);

    // When paused, blink the snake (400 ms on / 400 ms off) so the screen
    // visibly confirms drawing is still happening — distinguishing "paused"
    // from "frozen bug".
    bool snakeVisible = true;
    if (state == Paused) {
        snakeVisible = ((millis() / 400) & 1) != 0;
    }

    // Food is shown always — even on the "dark" half-beat — so the field has
    // a stable anchor you can see.
    display->fillRect(x + foodX * cellPx, y + foodY * cellPx, cellPx, cellPx);

    if (snakeVisible) {
        // Snake body (drawn tail-first so the head paints last and stays on top).
        for (int i = length - 1; i >= 0; --i) {
            int bx = x + body[i].x * cellPx;
            int by = y + body[i].y * cellPx;
            display->fillRect(bx, by, cellPx, cellPx);
        }
    }
}

bool SnakeGame::isBodyAt(uint8_t x, uint8_t y) const
{
    for (int i = 1; i < length; ++i) { // skip head; head will land on empty
        if (body[i].x == x && body[i].y == y)
            return true;
    }
    return false;
}

void SnakeGame::placeFood()
{
    // Pick a random empty cell.
    uint32_t start = millis();
    while (true) {
        uint8_t fx = random(0, W);
        uint8_t fy = random(0, H);
        bool occupied = false;
        for (int i = 0; i < length; ++i) {
            if (body[i].x == fx && body[i].y == fy) {
                occupied = true;
                break;
            }
        }
        if (!occupied) {
            foodX = fx;
            foodY = fy;
            return;
        }
        // Hard guard so a misplaced full board can't infinite-loop.
        if ((millis() - start) > 100)
            return;
    }
}

void SnakeGame::speedUp()
{
    // Mild ramp: every 3 foods shaves 15 ms, floor 70 ms.
    int tier = score / 30;
    int proposed = 180 - tier * 15;
    if (proposed < 70)
        proposed = 70;
    stepMs = (uint32_t)proposed;
}

#endif // ENABLE_GAMES_FRAME
