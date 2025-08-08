#ifndef __GAME_H
#define __GAME_H

#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL.h>

// Brick and world constants (moved from Game.cpp)
#define BRICK_XSIZE 80
#define BRICK_YSIZE 40
#define BRICK_WORLD_X_PADDING 10
#define BRICK_WORLD_Y_PADDING 10
#define BRICKS_X 10
#define BRICKS_Y 10
#define PADDLE_XSIZE 160
#define PADDLE_YSIZE 20
#define brick_x_pos(bricki) ((bricki)*((BRICK_WORLD_X_PADDING)+(BRICK_XSIZE)))
#define brick_y_pos(brickj) ((brickj)*((BRICK_WORLD_Y_PADDING)+(BRICK_YSIZE)))

struct Game {
    Game();
    ~Game();

    void mainLoop();
    void handleInput(double delta);
    void updateState(double delta);
    void renderFrame();
    void renderPaddle();
    void renderBall();
    void renderWorld();
    void renderBrickWorld();
    void moveBallWithInertia(double delta);
    void resetBall();
    void moveBall(int, int, double delta);
    void moveBallWithSpeed(int xdir, int ydir, double delta, double speed);
    void movePaddle(double, double delta);
    void setupBrickWorld();
    void destroyBrick(int,int);
    void detectCollisions();
    void switchBallInertia(int);

private:
    bool Running = false;
    bool ballInPlay = false;
    bool waitingToServe = true;
    SDL_Color brickColors[BRICKS_X][BRICKS_Y];
    int brickShapes[BRICKS_X][BRICKS_Y]; // 0=rect, 1=ellipse, 2=rounded rect
    int brickWallShape; // 0=rect, 1=ellipse, 2=rounded rect
    int brickHP[BRICKS_X][BRICKS_Y]; // hit points for each brick
    Uint32 lastPaddlePressTime[2]; // 0=left, 1=right, for double-tap detection
    double paddleCollisionCooldown; // cooldown after paddle hit
    bool justBouncedPaddle; // skip paddle collision for 1 frame
    bool ballHasClearedPaddle; // only allow paddle collision if ball has cleared paddle
};
#endif // __GAME_H
