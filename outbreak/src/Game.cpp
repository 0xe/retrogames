#include "Game.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>

#include <stdlib.h>
#include <stdio.h>

SDL_Window *win;
SDL_Renderer *ren;

SDL_Texture *bg_tex;
SDL_Texture *ball_tex;
SDL_Texture *paddle_tex;
SDL_Texture *brick_tex;

/* XXX(satish): Detect and set */
const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;

const char BALL_IMG[] = "ball.png";
const char PADDLE_IMG[] = "paddle.png";
const char BG_IMG[] = "bg.png";
const char BRICK_IMG[] = "brick.png";

const double BALL_MOVE = 400.0;
const int BALL_SIZE = 20;

const double PADDLE_MOVE = 600.0; // pixels/second, balanced
const int PADDLE_OVERFLOW = 120;
const double PADDLE_ACCL_DEF = 1;



double paddleposi = (SCREEN_WIDTH/2 - PADDLE_XSIZE/2), \
    paddleposj = (SCREEN_HEIGHT - PADDLE_YSIZE),
    paddleaccl = 1, paddleaccr = 1;
double prev_paddleposi = paddleposi;
double ballposi = paddleposi + PADDLE_XSIZE/2, \
    ballposj = paddleposj - PADDLE_YSIZE;
double prev_ballposi = ballposi, prev_ballposj = ballposj;
int valid_inertia_states[3] = {-1, 0, 1};
double ballinerti = 0, ballinertj = 0;

int brickWorld[BRICKS_X][BRICKS_Y];

/* TODO(satish): Error handling */
Game::Game() {
    SDL_Init(SDL_INIT_EVERYTHING);
    win = SDL_CreateWindow("OutBreak", 100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED); // Removed VSYNC for lower latency

    bg_tex = IMG_LoadTexture(ren, BG_IMG);
    ball_tex = IMG_LoadTexture(ren, BALL_IMG);
    paddle_tex = IMG_LoadTexture(ren, PADDLE_IMG);
    brick_tex = IMG_LoadTexture(ren, BRICK_IMG);

    mainLoop();
}

Game::~Game() {
    SDL_DestroyTexture(bg_tex);
    SDL_DestroyTexture(ball_tex);
    SDL_DestroyTexture(paddle_tex);
    SDL_DestroyTexture(brick_tex);
    SDL_DestroyWindow(win);
    SDL_DestroyRenderer(ren);
    SDL_Quit();
}

void renderTexture(SDL_Texture *tex, SDL_Renderer *ren, int x, int y, int w, int h) {
    SDL_Rect dest;
    dest.x = x; dest.y = y; dest.w = w; dest.h = h;
    SDL_RenderCopy(ren, tex, NULL, &dest);
}

void Game::switchBallInertia(int wpos) {

    if (wpos == 1) {
        if(ballinerti == -1 and ballinertj == 1) {
            ballinerti = -1; ballinertj = -1;
        } else if(ballinerti == -1 && ballinertj == -1) {
            ballinerti = -1; ballinertj = 1;
        } else if(ballinerti == 1 && ballinertj == -1) {
            ballinerti = 1; ballinertj = 1;
        } else if(ballinerti == 1 && ballinertj == 1) {
            ballinerti = 1; ballinertj = -1;
        }
    } else if (wpos == 0) {
       if(ballinerti == -1 and ballinertj == 1) {
            ballinerti = 1; ballinertj = 1;
        } else if(ballinerti == -1 && ballinertj == -1) {
            ballinerti = 1; ballinertj = -1;
        } else if(ballinerti == 1 && ballinertj == 1) {
            ballinerti = -1; ballinertj = 1;
        } else if(ballinerti == 1 && ballinertj == -1) {
            ballinerti = -1; ballinertj = -1;
       }
    }
}

void Game::detectCollisions() {
    if (!ballInPlay) return;
    // Paddle collision (AABB)
    if (!ballHasClearedPaddle) return; // Only allow paddle collision if ball has cleared paddle
    if (paddleCollisionCooldown > 0) return; // Skip paddle collision if cooldown is active
    bool paddle_x = (ballposi + BALL_SIZE > paddleposi) && (ballposi < paddleposi + PADDLE_XSIZE);
    bool paddle_y = (ballposj + BALL_SIZE > paddleposj) && (ballposj < paddleposj + PADDLE_YSIZE);
    if (paddle_x && paddle_y) {
        // Move ball just above paddle and give strong upward velocity
        // Move ball well above the paddle to avoid re-collision
        ballposj = paddleposj - BALL_SIZE - PADDLE_YSIZE;
        // Reflect based on where it hit the paddle (granular)
        double hit_pos = (ballposi + BALL_SIZE/2.0) - (paddleposi + PADDLE_XSIZE/2.0);
        double norm = hit_pos / (PADDLE_XSIZE/2.0); // -1 (left) to 1 (right)
        int new_xdir = (int)round(norm * 2); // -2, -1, 0, 1, 2
        if (new_xdir == 0) new_xdir = (rand()%2==0) ? -1 : 1; // avoid vertical lock
        ballinerti = new_xdir;
        // Enforce minimum horizontal velocity for escape
        if (fabs(ballinerti) < 1e-2) {
            ballinerti = (rand()%2==0) ? -1 : 1;
        } else if (abs(ballinerti) < 1) {
            ballinerti = (ballinerti < 0) ? -1 : 1;
        }
        // Add a small random nudge if exactly zero
        if (ballinerti == 0) ballinerti = (rand()%2==0) ? -1 : 1;
        // Strong springy rebound: always a strong upward direction
        ballinertj = -1.5;
        if (std::isnan(ballinertj) || std::isinf(ballinertj) || fabs(ballinertj) < 1e-3) {
            printf("[DEBUG] ballinertj nan/inf/zero in detectCollisions (paddle), resetting to -1.5\n");
            ballinertj = -1.5;
        }
        printf("[DEBUG] After paddle bounce: ballposi=%.2f ballposj=%.2f ballinerti=%.2f ballinertj=%.2f\n", ballposi, ballposj, ballinerti, ballinertj);

        justBouncedPaddle = true;
        ballHasClearedPaddle = false;
        paddleCollisionCooldown = 0.12; // 120 ms cooldown to prevent sticking
        if (std::isnan(ballinertj) || std::isinf(ballinertj) || fabs(ballinertj) < 1e-3) {
            printf("[DEBUG] ballinertj was nan, inf, or zero, resetting to -3.5\n");
            ballinertj = -3.5;
        }
        if (std::isnan(ballinerti) || std::isinf(ballinerti)) {
            printf("[DEBUG] ballinerti was nan or inf, resetting to random direction\n");
            ballinerti = (rand()%2==0) ? -1 : 1;
        }
        printf("[DEBUG] Paddle collision: ballposj=%.2f paddleposj=%.2f ballinertj=%.2f\n", ballposj, paddleposj, ballinertj);
        return;
    }

    // Wall collisions
    if (ballposi <= 0) {
        ballposi = 0;
        ballinerti = 1;
    } else if ((ballposi + BALL_SIZE) >= SCREEN_WIDTH) {
        ballposi = SCREEN_WIDTH - BALL_SIZE;
        ballinerti = -1;
    }
    if (ballposj <= 0) {
        ballposj = 0;
        ballinertj = 1;
        if (std::isnan(ballinertj) || std::isinf(ballinertj)) { printf("[DEBUG] ballinertj nan/inf in detectCollisions (brick), resetting to 1\n"); ballinertj = 1; }
    } else if ((ballposj + BALL_SIZE) >= SCREEN_HEIGHT) {
        resetBall();
        return;
    }

    // Brick collisions (AABB)
    int wall_width = BRICKS_X * BRICK_XSIZE + (BRICKS_X - 1) * BRICK_WORLD_X_PADDING;
    int start_x = (SCREEN_WIDTH - wall_width) / 2;
    for(int j=0; j<BRICKS_Y; j++) {
        for(int i=0; i<BRICKS_X; i++) {
            if(brickWorld[i][j] != 0) {
                int brick_x = start_x + i * (BRICK_XSIZE + BRICK_WORLD_X_PADDING);
                int brick_y = BRICK_WORLD_Y_PADDING + j * (BRICK_YSIZE + BRICK_WORLD_Y_PADDING);
                bool x_collides = (ballposi + BALL_SIZE > brick_x && ballposi < (brick_x + BRICK_XSIZE));
                bool y_collides = (ballposj + BALL_SIZE > brick_y && ballposj < (brick_y + BRICK_YSIZE));
                if (x_collides && y_collides) {
                    destroyBrick(i,j);
                    // Push ball out of brick
                    if (prev_ballposj + BALL_SIZE <= brick_y) {
                        // Came from above
                        ballposj = brick_y - BALL_SIZE;
                        ballinertj = -ballinertj;
                    } else if (prev_ballposj >= brick_y + BRICK_YSIZE) {
                        // Came from below
                        ballposj = brick_y + BRICK_YSIZE;
                        ballinertj = -ballinertj;
                    } else if (prev_ballposi + BALL_SIZE <= brick_x) {
                        // Came from left
                        ballposi = brick_x - BALL_SIZE;
                        ballinerti = -ballinerti;
                    } else if (prev_ballposi >= brick_x + BRICK_XSIZE) {
                        // Came from right
                        ballposi = brick_x + BRICK_XSIZE;
                        ballinerti = -ballinerti;
                    } else {
                        // Default: vertical bounce
                        ballinertj = -ballinertj;
                    }
                    return;
                }
            }
        }
    }
}

/*
  Ball movement:
    A
  \ | /
<-- o -->
  / | \
    V

 (-1, 0, 1)

 x  y
 0   0 (no movement)
 0   1 (move up)
 0  -1 (move down)
 1   0 (move right)
 -1  0 (move left)
 -1 -1 (move downleft)
 -1  1 (move upleft)
 1  -1 (move downright)
 1   1 (move upright)
 */
void Game::moveBall(int xdir, int ydir, double delta) {
    double move_amount = BALL_MOVE * delta;
    if (xdir == 0 && ydir == 0) {          // do nothing
    } else if (xdir == 0 && ydir == 1) {   // move down
        ballposj = ((ballposj + move_amount) < SCREEN_HEIGHT) ? (ballposj + move_amount) : (SCREEN_HEIGHT - BALL_SIZE);
    } else if (xdir == 0 && ydir == -1) {  // move up
        ballposj = ((ballposj - move_amount) > 0) ? (ballposj - move_amount) : 0;
    } else if (xdir == -1 && ydir == 0) {  // move left
        ballposi = ((ballposi - move_amount) > 0) ? (ballposi - move_amount) : 0;
    } else if (xdir == 1 && ydir == 0) {   // move right
        ballposi = ((ballposi + move_amount) < SCREEN_WIDTH)? (ballposi + move_amount): (SCREEN_WIDTH-BALL_SIZE);
    } else if (xdir == -1 && ydir == 1) {  // move upleft
        ballposj = ((ballposj + move_amount) < SCREEN_HEIGHT) ? (ballposj + move_amount) : (SCREEN_HEIGHT - BALL_SIZE);
        ballposi = ((ballposi - move_amount) > 0) ? (ballposi - move_amount) : 0;
    } else if (xdir == -1 && ydir == -1) { // move downleft
        ballposj = ((ballposj - move_amount) > 0) ? (ballposj - move_amount) : 0;
        ballposi = ((ballposi - move_amount) > 0) ? (ballposi - move_amount) : 0;
    } else if (xdir == 1 && ydir == -1) {  // move downright
        ballposj = ((ballposj - move_amount) > 0) ? (ballposj - move_amount) : 0;
        ballposi = ((ballposi + move_amount) < SCREEN_WIDTH)? (ballposi + move_amount): (SCREEN_WIDTH-BALL_SIZE);
    } else if (xdir == 1 && ydir == 1) {   // move upright
        ballposj = ((ballposj + move_amount) < SCREEN_HEIGHT) ? (ballposj + move_amount) : (SCREEN_HEIGHT - BALL_SIZE);
        ballposi = ((ballposi + move_amount) < SCREEN_WIDTH)? (ballposi + move_amount): (SCREEN_WIDTH-BALL_SIZE);
    }
}

void Game::resetBall() {
    ballInPlay = false;
    waitingToServe = true;
    paddleposi = (SCREEN_WIDTH/2 - PADDLE_XSIZE/2);
    paddleposj = (SCREEN_HEIGHT - PADDLE_YSIZE);
    ballposi = paddleposi + PADDLE_XSIZE/2;
    ballposj = paddleposj - PADDLE_YSIZE;
    ballinerti = 0;
    ballinertj = 0;
    if (std::isnan(ballinertj) || std::isinf(ballinertj)) { printf("[DEBUG] ballinertj nan/inf in resetBall, resetting to 0\n"); ballinertj = 0; }
    setupBrickWorld();
}

// <- ooooo ->
void Game::movePaddle(double xdir, double delta) {
    if (!ballInPlay && !waitingToServe) {
        // Prevent paddle and ball movement after game over
        return;
    }
    double paddle_mov_right = PADDLE_MOVE * paddleaccr * delta;
    double paddle_mov_left = PADDLE_MOVE * paddleaccl * delta;
    if (xdir == 1) { // move right
        paddleposi += paddle_mov_right;
    } else if (xdir == -1) { // move left
        paddleposi -= paddle_mov_left;
    }
    // Clamp paddle to screen bounds
    if (paddleposi < 0) paddleposi = 0;
    if (paddleposi > SCREEN_WIDTH - PADDLE_XSIZE) paddleposi = SCREEN_WIDTH - PADDLE_XSIZE;
    // Clamp paddle vertically as well (should always be at bottom)
    paddleposj = SCREEN_HEIGHT - PADDLE_YSIZE;
    if (waitingToServe) {
        ballposi = paddleposi + PADDLE_XSIZE/2;
        ballposj = paddleposj - PADDLE_YSIZE;
    }
}

void Game::setupBrickWorld() {
    // Only use rounded rectangles for bricks
    brickWallShape = 2;
    for(int j=0; j<BRICKS_Y; j++) {
        for(int i=0; i<BRICKS_X; i++) {
            if (rand() % 100 < 15) {
                brickWorld[i][j] = 0;
            } else {
                brickWorld[i][j] = 1;
                brickHP[i][j] = 2; // 2 hits to break
                // Random color
                brickColors[i][j].r = rand() % 256;
                brickColors[i][j].g = rand() % 256;
                brickColors[i][j].b = rand() % 256;
                brickColors[i][j].a = 255;
            }
        }
    }
}

void Game::destroyBrick(int i, int j) {
    if (brickHP[i][j] > 1) {
        brickHP[i][j]--;
    } else {
        brickWorld[i][j] = 0;
        brickHP[i][j] = 0;
    }
}

void Game::handleInput(double delta) {
    // Only restrict input if not waitingToServe and not ballInPlay
    if (!ballInPlay && !waitingToServe) {
        // Ignore all paddle movement input if game is over
        return;
    }
    // Double-tap detection for acceleration burst
    static const double DOUBLE_TAP_WINDOW = 0.22; // seconds
    static double paddleBoostTimer = 0.0;
    static int paddleBoostDir = 0; // -1=left, 1=right
    Uint32 now = SDL_GetTicks();

    SDL_Event event;
    // Handle discrete events (like quit)
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            Running = false;
            break;
        }
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                Running = false;
                break;
            }
        }
    }

    const Uint8* keyState = SDL_GetKeyboardState(NULL);
    bool left = keyState[SDL_SCANCODE_LEFT];
    bool right = keyState[SDL_SCANCODE_RIGHT];

    // Double-tap logic
    double speedMultiplier = 1.0;
    if (left && !right) {
        if (now - lastPaddlePressTime[0] < DOUBLE_TAP_WINDOW * 1000) {
            paddleBoostTimer = 0.18; // 180 ms boost
            paddleBoostDir = -1;
        }
        lastPaddlePressTime[0] = now;
    } else if (right && !left) {
        if (now - lastPaddlePressTime[1] < DOUBLE_TAP_WINDOW * 1000) {
            paddleBoostTimer = 0.18;
            paddleBoostDir = 1;
        }
        lastPaddlePressTime[1] = now;
    }
    if (paddleBoostTimer > 0) {
        speedMultiplier = 1.8;
        paddleBoostTimer -= delta;
        if ((left && paddleBoostDir == -1) || (right && paddleBoostDir == 1)) {
            // keep boosting
        } else {
            paddleBoostTimer = 0;
        }
    }
    // Move paddle with boost
    if (left && !right) {
        paddleaccl = 1.0;
        paddleaccr = 1.0;
        movePaddle(-1, delta * speedMultiplier);
    } else if (right && !left) {
        paddleaccl = 1.0;
        paddleaccr = 1.0;
        movePaddle(1, delta * speedMultiplier);
    } else {
        paddleaccl = 1.0;
        paddleaccr = 1.0;
    }

    if (keyState[SDL_SCANCODE_UP] && waitingToServe) {
        ballposi = paddleposi + PADDLE_XSIZE/2;
        ballposj = paddleposj - PADDLE_YSIZE;
        ballinerti = 1; ballinertj = -1;
        ballInPlay = true;
        waitingToServe = false;
    }
    // (If you want to handle DOWN key, add code here)
}

// Multi-mode animated backgrounds
#include <vector>
#include <cmath>
struct Star { float x, y, speed, alpha, alpha_dir; };
static std::vector<Star> stars;
static bool stars_initialized = false;
static int backgroundMode = 0; // 0=blinking stars, 1=color cycle, 2=stripes, 3=rainbow
static Uint32 bg_start_ticks = 0;

void Game::renderWorld() {
    if (!stars_initialized) {
        // Pick a random mode at game start
        backgroundMode = rand() % 4;
        bg_start_ticks = SDL_GetTicks();
        // Init stars
        stars.clear();
        for (int i = 0; i < 80; ++i) {
            stars.push_back({(float)(rand()%SCREEN_WIDTH), (float)(rand()%SCREEN_HEIGHT), 0.5f + 2.5f*(rand()/(float)RAND_MAX), (float)(128 + rand()%128), (rand()%2) ? 1.0f : -1.0f});
        }
        stars_initialized = true;
    }
    Uint32 ticks = SDL_GetTicks() - bg_start_ticks;
    switch (backgroundMode) {
        case 0: // Blinking (twinkling) stars
        {
            SDL_SetRenderDrawColor(ren, 12, 16, 32, 255); // Slightly lighter, blue-gray
            SDL_RenderClear(ren);
            for (auto& s : stars) {
                s.y += s.speed;
                if (s.y > SCREEN_HEIGHT) { s.y = 0; s.x = rand()%SCREEN_WIDTH; s.speed = 0.5f + 2.5f*(rand()/(float)RAND_MAX); }
                // Twinkle
                s.alpha += s.alpha_dir * (1.5f + rand()%2) * (0.6f + 0.4f * sinf(ticks/800.0f));
                if (s.alpha > 255) { s.alpha = 255; s.alpha_dir = -1.0f; }
                if (s.alpha < 80) { s.alpha = 80; s.alpha_dir = 1.0f; }
                filledCircleRGBA(ren, (int)s.x, (int)s.y, 1 + (int)(s.speed/1.5), 180, 200, 255, (Uint8)(s.alpha * 0.7)); // blue-white, lower alpha
            }
            break;
        }
        case 1: // Color cycling
        {
            float t = ticks / 1000.0f;
            // Lower amplitude for pastel/dark effect
            Uint8 r = (Uint8)(36 + 60 * sin(t));
            Uint8 g = (Uint8)(36 + 60 * sin(t + 2));
            Uint8 b = (Uint8)(56 + 60 * sin(t + 4));
            SDL_SetRenderDrawColor(ren, r, g, b, 255);
            SDL_RenderClear(ren);
            break;
        }
        case 2: // Moving stripes
        {
            SDL_SetRenderDrawColor(ren, 16, 20, 36, 255);
            SDL_RenderClear(ren);
            int stripe_height = 32;
            int offset = (ticks/8) % (2*stripe_height);
            for (int y = -stripe_height; y < SCREEN_HEIGHT+stripe_height; y += 2*stripe_height) {
                boxRGBA(ren, 0, y+offset, SCREEN_WIDTH, y+stripe_height+offset, 60, 80, 120, 38); // more transparent, muted blue
            }
            break;
        }
        case 3: // Rainbow gradient
        {
            for (int y = 0; y < SCREEN_HEIGHT; ++y) {
                float t = (ticks/700.0f) + (float)y/SCREEN_HEIGHT*6.28f;
                Uint8 r = (Uint8)(36 + 60 * sin(t));
                Uint8 g = (Uint8)(36 + 60 * sin(t + 2));
                Uint8 b = (Uint8)(56 + 60 * sin(t + 4));
                SDL_SetRenderDrawColor(ren, r, g, b, 255);
                SDL_RenderDrawLine(ren, 0, y, SCREEN_WIDTH, y);
            }
            break;
        }
    }
}

void Game::renderBall() {
    // Draw main ball
    filledCircleRGBA(ren, (int)(ballposi + BALL_SIZE/2), (int)(ballposj + BALL_SIZE/2), BALL_SIZE/2, 255, 80, 30, 255);
    // Draw sheen (arc)
    filledEllipseRGBA(ren, (int)(ballposi + BALL_SIZE/2 - BALL_SIZE/5), (int)(ballposj + BALL_SIZE/2 - BALL_SIZE/5), BALL_SIZE/3, BALL_SIZE/7, 255, 255, 255, 120);
}

void Game::renderPaddle() {
    // Draw main paddle
    roundedBoxRGBA(ren, (int)paddleposi, (int)paddleposj, (int)(paddleposi + PADDLE_XSIZE), (int)(paddleposj + PADDLE_YSIZE), 8, 255, 200, 50, 255);
    // Draw paddle sheen (top highlight)
    boxRGBA(ren, (int)paddleposi + 10, (int)paddleposj + 3, (int)(paddleposi + PADDLE_XSIZE - 10), (int)(paddleposj + PADDLE_YSIZE/3), 255, 255, 255, 70);
}

void Game::renderBrickWorld() {
    // Calculate total wall width for centering
    int wall_width = BRICKS_X * BRICK_XSIZE + (BRICKS_X - 1) * BRICK_WORLD_X_PADDING;
    int start_x = (SCREEN_WIDTH - wall_width) / 2;
    int radius = 10;
    for(int j=0; j<BRICKS_Y; j++) {
        for(int i=0; i<BRICKS_X; i++) {
            if(brickWorld[i][j] != 0) {
                int x = start_x + i * (BRICK_XSIZE + BRICK_WORLD_X_PADDING);
                int y = BRICK_WORLD_Y_PADDING + j * (BRICK_YSIZE + BRICK_WORLD_Y_PADDING);
                SDL_Color c = brickColors[i][j];
                if (brickHP[i][j] == 1) {
                    // Darken color for damaged brick
                    c.r = (Uint8)(c.r * 0.5);
                    c.g = (Uint8)(c.g * 0.5);
                    c.b = (Uint8)(c.b * 0.5);
                }
                // Optional: glow effect
                roundedBoxRGBA(ren, x-4, y-4, x+BRICK_XSIZE+4, y+BRICK_YSIZE+4, radius+6, c.r, c.g, c.b, 40);
                // Main brick
                roundedBoxRGBA(ren, x, y, x+BRICK_XSIZE, y+BRICK_YSIZE, radius, c.r, c.g, c.b, c.a);
                // Shine: overlay a lighter semi-transparent rounded rect at the top-left
                Uint8 shine_r = (Uint8)std::min<unsigned int>(255, c.r + 80);
                Uint8 shine_g = (Uint8)std::min<unsigned int>(255, c.g + 80);
                Uint8 shine_b = (Uint8)std::min<unsigned int>(255, c.b + 80);
                int shine_w = BRICK_XSIZE * 0.6;
                int shine_h = BRICK_YSIZE * 0.4;
                roundedBoxRGBA(ren, x+3, y+3, x+shine_w, y+shine_h, radius/2, shine_r, shine_g, shine_b, 120);
            }
        }
    }
}

void Game::updateState(double delta) {
    justBouncedPaddle = false;
    prev_ballposi = ballposi;
    prev_ballposj = ballposj;
    prev_paddleposi = paddleposi;
    // Decrement paddle collision cooldown
    if (paddleCollisionCooldown > 0) {
        paddleCollisionCooldown -= delta;
        if (paddleCollisionCooldown < 0) paddleCollisionCooldown = 0;
    }
    // Allow paddle collision only after ball has cleared the paddle
    if (ballposj + BALL_SIZE < paddleposj - 20) ballHasClearedPaddle = true; // Increase clearance threshold
    detectCollisions();
    moveBallWithInertia(delta);
}

/*
  Renders the ball with inertia:

    lrd            rlu
 ===     ===     o     o
  o   =>  o     o   =>  o
 o         o   ===     ===

    rld            lru
 ===     ===   o         o
  o   =>  o     o   =>  o
   o     o     ===     ===

    lru            rld
 |       | o   | o     |
 |o   => |o    |o   => |o
 | o     |     |       | o

    lru            rld

 o |  =>  |    |   => o |
  o|     o|   o|       o|
   |    o |  o |        |

   rld: moveBall(-1, 1)
   lrd: moveBall(1, 1)

   rlu: moveBall(-1, -1)
   lru: moveBall(1, -1)

   nop: moveBall(current, current)
*/
void Game::moveBallWithInertia(double delta) {
    // Default speed
    double ball_speed = BALL_MOVE;
    // If the ball just bounced on the paddle, give it a speed boost
    if (justBouncedPaddle) {
        ball_speed *= 1.5; // Spring boost after paddle hit
    }
    // If the ball is above the paddle and moving down, and paddle is moving toward the ball, slow the ball
    bool close_to_paddle = (ballposj + BALL_SIZE > paddleposj - 1.5*PADDLE_YSIZE) && (ballposj < paddleposj);
    bool ball_moving_down = (ballinertj == 1);
    // Estimate paddle velocity (difference over last frame)
    double paddle_velocity = paddleposi - prev_paddleposi;
    bool paddle_approaching = (paddle_velocity > 0 && ballposi > paddleposi) || (paddle_velocity < 0 && ballposi < paddleposi);
    if (close_to_paddle && ball_moving_down && paddle_approaching) {
        ball_speed *= 0.5; // Slow down
    }
    moveBallWithSpeed(ballinerti, ballinertj, delta, ball_speed);
}

// Helper for custom ball speed
void Game::moveBallWithSpeed(int xdir, int ydir, double delta, double speed) {
    double move_amount = speed * delta;
    if (xdir == 0 && ydir == 0) {          // do nothing
    } else if (xdir == 0 && ydir == 1) {   // move down
        ballposj = ((ballposj + move_amount) < SCREEN_HEIGHT) ? (ballposj + move_amount) : (SCREEN_HEIGHT - BALL_SIZE);
    } else if (xdir == 0 && ydir == -1) {  // move up
        ballposj = ((ballposj - move_amount) > 0) ? (ballposj - move_amount) : 0;
    } else if (xdir == -1 && ydir == 0) {  // move left
        ballposi = ((ballposi - move_amount) > 0) ? (ballposi - move_amount) : 0;
    } else if (xdir == 1 && ydir == 0) {   // move right
        ballposi = ((ballposi + move_amount) < SCREEN_WIDTH)? (ballposi + move_amount): (SCREEN_WIDTH-BALL_SIZE);
    } else if (xdir == -1 && ydir == 1) {  // move upleft
        ballposj = ((ballposj + move_amount) < SCREEN_HEIGHT) ? (ballposj + move_amount) : (SCREEN_HEIGHT - BALL_SIZE);
        ballposi = ((ballposi - move_amount) > 0) ? (ballposi - move_amount) : 0;
    } else if (xdir == -1 && ydir == -1) { // move downleft
        ballposj = ((ballposj - move_amount) > 0) ? (ballposj - move_amount) : 0;
        ballposi = ((ballposi - move_amount) > 0) ? (ballposi - move_amount) : 0;
    } else if (xdir == 1 && ydir == -1) {  // move downright
        ballposj = ((ballposj - move_amount) > 0) ? (ballposj - move_amount) : 0;
        ballposi = ((ballposi + move_amount) < SCREEN_WIDTH)? (ballposi + move_amount): (SCREEN_WIDTH-BALL_SIZE);
    } else if (xdir == 1 && ydir == 1) {   // move upright
        ballposj = ((ballposj + move_amount) < SCREEN_HEIGHT) ? (ballposj + move_amount) : (SCREEN_HEIGHT - BALL_SIZE);
        ballposi = ((ballposi + move_amount) < SCREEN_WIDTH)? (ballposi + move_amount): (SCREEN_WIDTH-BALL_SIZE);
    }
}

void Game::renderFrame() {
    // SDL_RenderClear(ren); // No longer needed, handled by renderWorld
    renderWorld(); // Now handles background and animation
    renderBrickWorld();
    renderBall();
    renderPaddle();
    SDL_RenderPresent(ren);
}

/*
  - Runs at 60 fps
  - Gets input
  - Updates game state
  - Renders a frame
*/
void Game::mainLoop() {
    Running = true;
    setupBrickWorld();

    Uint32 last_time = SDL_GetTicks();

    while (Running) {
        Uint32 current_time = SDL_GetTicks();
        double delta = (current_time - last_time) / 1000.0; // seconds
        if (delta > 0.05) delta = 0.05; // clamp to avoid huge jumps
        last_time = current_time;

        handleInput(delta);
        updateState(delta);
        renderFrame();
    }
}

