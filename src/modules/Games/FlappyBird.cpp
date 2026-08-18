#include "FlappyBird.h"
#include "DebugConfiguration.h"
#include "graphics/ScreenFonts.h"
#include <OLEDDisplay.h>
#include <Arduino.h>

#ifdef ENABLE_GAMES_FRAME

FlappyBird::FlappyBird()
    : birdX(20), birdY(16), velocity(0), pipeHead(0),
      score(0), bestScore(0), stepMs(40), spawnTimer(0), started(false)
{
    for (int i = 0; i < MAX_PIPES; ++i) {
        pipes[i].x = -1;
        pipes[i].gapY = 0;
        pipes[i].scored = true;
    }
    reset();
}

void FlappyBird::reset()
{
    birdX = 20;
    birdY = 16;          // ~1/4 from top — leaves headroom for upward flaps
    velocity = 0;
    score = 0;
    stepMs = 40;         // 25 fps — a touch slower so taps feel less twitchy
    pipeHead = 0;
    spawnTimer = PIPE_SPACING; // first spawn fires after PIPE_SPACING frames
    started = false;     // wait for first flap before applying gravity
    for (int i = 0; i < MAX_PIPES; ++i) {
        pipes[i].x = -1;
        pipes[i].gapY = 0;
        pipes[i].scored = true;
    }
    state = Idle;
    // Seed first pipe so player has something to aim at as soon as they start.
    spawnPipe();
}

void FlappyBird::start()
{
    reset();
    state = Playing;
}

void FlappyBird::pause()
{
    if (state == Playing)
        state = Paused;
}

void FlappyBird::resume()
{
    if (state == Paused)
        state = Playing;
}

void FlappyBird::gameOver()
{
    state = GameOver;
    if (score > bestScore)
        bestScore = score;
}

void FlappyBird::flap()
{
    if (state != Playing)
        return;
    // First tap also "launches" the bird (started flag gates physics in step()).
    // Without this the player would walk into an immediately-running game and
    // die before they can read the screen.
    started = true;
    // Upward impulse. v1 was -6; halved to -3 still felt too high; tuned to -4
    // so a single tap lifts the bird ~4 px with a tighter, more controllable tap rhythm.
    velocity = -4;
}

void FlappyBird::spawnPipe()
{
    // Find an empty slot (ring buffer, linear search — MAX_PIPES is tiny).
    int slot = -1;
    for (int i = 0; i < MAX_PIPES; ++i) {
        if (pipes[i].x < 0) {
            slot = i;
            break;
        }
    }
    if (slot < 0)
        return; // no free slot — drop this spawn (rare; budget is 4 pipes)
    uint8_t gapY = (uint8_t)random(6, H - PIPE_GAP - 6);
    pipes[slot].x = W;
    pipes[slot].gapY = gapY;
    pipes[slot].scored = false;
    pipeHead = (slot + 1) % MAX_PIPES;
}

void FlappyBird::step()
{
    if (state != Playing)
        return;

    // Wait for the first flap before doing anything. The bird hovers in place
    // and pipes still scroll, so the player can read the layout before reacting.
    if (!started) {
        return;
    }

    // --- Physics ----------------------------------------------------------
    // Gravity is +1 per frame, terminal velocity clamped to 2 (was 3 — kept
    // moving down in lockstep with the flap impulse so the difficulty curve
    // stays balanced). 40 ms cadence + lower terminal = more reaction room.
    velocity += 1;
    if (velocity > 2)
        velocity = 2;
    int16_t newY = birdY + velocity;
    if (newY < 0)
        newY = 0;
    if (newY > H - BIRD_H)
        newY = H - BIRD_H;
    birdY = newY;

    // Floor / ceiling kill.
    if (birdY <= 0 || (birdY + BIRD_H) >= H) {
        gameOver();
        return;
    }

    // --- Pipes ------------------------------------------------------------
    // Scroll every pipe left by 1 px; recycle a slot when fully off-screen.
    for (int i = 0; i < MAX_PIPES; ++i) {
        if (pipes[i].x >= 0) {
            pipes[i].x -= 1;
            if (pipes[i].x + PIPE_W < 0) {
                pipes[i].x = -1; // recycle slot
            }
        }
    }

    // Spawn pacing: every PIPE_SPACING frames, push a new pipe at x=W.
    if (spawnTimer > 0)
        spawnTimer--;
    if (spawnTimer == 0) {
        spawnPipe();
        spawnTimer = PIPE_SPACING;
    }

    // --- Collision with pipes --------------------------------------------
    for (int i = 0; i < MAX_PIPES; ++i) {
        if (pipes[i].x < 0)
            continue;
        if (birdHits((uint8_t)birdX, (uint8_t)birdY, BIRD_W, BIRD_H, pipes[i].x, pipes[i].gapY,
                     (uint8_t)PIPE_W, (uint8_t)PIPE_GAP)) {
            gameOver();
            return;
        }
        // Score when bird's leading edge crosses the pipe's trailing edge.
        if (!pipes[i].scored && (birdX > (pipes[i].x + PIPE_W))) {
            score++;
            pipes[i].scored = true;
        }
    }
}

bool FlappyBird::birdHits(uint8_t bx, uint8_t by, uint8_t bw, uint8_t bh, int8_t px, uint8_t py,
                          uint8_t pw, uint8_t gapH) const
{
    // Horizontal overlap.
    if ((int)bx + bw <= (int)px)
        return false;
    if ((int)bx >= (int)px + pw)
        return false;
    // Gap is between py (top of gap) and py+gapH (just below gap → bottom pipe top).
    int gapTop = (int)py;
    int gapBottom = (int)py + (int)gapH;
    // Bird overlaps top pipe if bird's top is above the gap.
    if ((int)by < gapTop)
        return true;
    // Bird overlaps bottom pipe if bird's bottom is below the gap.
    if ((int)by + (int)bh > gapBottom)
        return true;
    return false;
}

void FlappyBird::draw(OLEDDisplay *display, int16_t x, int16_t y)
{
    // Defensive clear so the field never retains stale pipe pixels from a prior
    // state (e.g. when paused at frame N and pipe x shifted, the previous frame
    // is no longer visible). GamesModule.draw() also clears before this, but
    // keep the behavior self-contained.
    display->clear();

    // 1-pixel border lifted 2 px from the bottom (matches SnakeGame layout).
    display->drawRect(x, y, W, H - 2);

    // Pipes: top segment + bottom segment drawn from same (x, gapY).
    for (int i = 0; i < MAX_PIPES; ++i) {
        if (pipes[i].x < 0)
            continue;
        const int px = x + pipes[i].x;
        const int gapTop = y + pipes[i].gapY;
        const int gapBot = y + pipes[i].gapY + PIPE_GAP;
        // Top pipe: from y to gapTop-1.
        if (gapTop > y)
            display->fillRect(px, y, PIPE_W, gapTop - y);
        // Bottom pipe: from gapBot to bottom.
        int botH = (y + H - 2) - gapBot;
        if (botH > 0)
            display->fillRect(px, gapBot, PIPE_W, botH);
    }

    // Bird: 4x4 px block. Center bird vertically on screen by using birdY
    // directly as top-left.
    display->fillRect(x + birdX, y + birdY, BIRD_W, BIRD_H);

    // "PRESS UP" prompt while waiting for the first flap. Blinks at 2 Hz so
    // it's obvious without being annoying. The string is small enough to fit
    // in 100 px wide game area (8 chars * 6 px ≈ 48 px, centered).
    if (!started && state != GameOver) {
        display->setTextAlignment(TEXT_ALIGN_CENTER);
        display->setFont(FONT_SMALL);
        bool show = ((millis() / 250) & 1) == 0; // 2 Hz blink, on-250ms / off-250ms
        if (show) {
            display->drawString(x + W / 2, y + 50, "PRESS UP");
        }
        display->setTextAlignment(TEXT_ALIGN_LEFT);
    }
}

#endif // ENABLE_GAMES_FRAME

#ifndef ENABLE_GAMES_FRAME
// Stub constructor for builds without ENABLE_GAMES_FRAME.
// GamesModule still contains a FlappyBird member, so this symbol must exist.
FlappyBird::FlappyBird()
    : birdX(0), birdY(0), velocity(0), pipeHead(0),
      score(0), bestScore(0), stepMs(0), spawnTimer(0), started(false)
{
}
#endif // !ENABLE_GAMES_FRAME
