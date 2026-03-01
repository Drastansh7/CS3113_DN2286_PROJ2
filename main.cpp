/**
* Author: Drastansh Nadola
* Assignment: Pong Clone
* Date due: 02/28/2026
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "CS3113/cs3113.h"
#include <cmath>

// screen
constexpr int SCREEN_WIDTH  = 1280,
              SCREEN_HEIGHT = 720,
              FPS           = 60;

constexpr char BG_COLOUR[] = "#0F0F23";
constexpr Vector2 ORIGIN = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

// paddle config
constexpr float PADDLE_WIDTH = 30.0f,
                PADDLE_HEIGHT = 128.0f,
                PADDLE_SPEED = 400.0f,
                PADDLE_MARGIN = 50.0f,
                AI_SPEED = 350.0f; // slightly slower so AI is beatable

// ball config
constexpr float BALL_SIZE  = 24.0f,
                BALL_SPEED = 400.0f;
constexpr int MAX_BALLS = 3;

constexpr float WIN_MSG_SCALE = 2.0f;

// asset file paths
constexpr char PADDLE1_FP[] = "assets/paddle1.png";
constexpr char PADDLE2_FP[] = "assets/paddle2.png";
constexpr char BALL_FP[]    = "assets/ball.png";
constexpr char P1_WINS_FP[] = "assets/p1_wins.png";
constexpr char P2_WINS_FP[] = "assets/p2_wins.png";

// globals
AppStatus gAppStatus     = RUNNING;
float     gPreviousTicks = 0.0f;

Vector2 gPaddle1Position = { PADDLE_MARGIN, ORIGIN.y };
Vector2 gPaddle2Position = { SCREEN_WIDTH - PADDLE_MARGIN, ORIGIN.y };
Vector2 gPaddleScale     = { PADDLE_WIDTH, PADDLE_HEIGHT };
float gPaddle1Movement = 0.0f;
float gPaddle2Movement = 0.0f;

Vector2 gBallPositions[MAX_BALLS];
Vector2 gBallVelocities[MAX_BALLS];
Vector2 gBallScale = { BALL_SIZE, BALL_SIZE };
bool    gBallActive[MAX_BALLS] = { true, false, false };
int     gActiveBallCount = 1;

bool gSinglePlayerMode = false;
bool gGameOver = false;
int  gWinner   = 0; // 0 = nobody, 1 = left player, 2 = right player

Texture2D gPaddle1Texture, gPaddle2Texture, gBallTexture;
Texture2D gP1WinsTexture, gP2WinsTexture;

// func declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();
void resetBalls();
void renderObject(const Texture2D *texture, const Vector2 *position,
                  const Vector2 *scale);
bool isColliding(const Vector2 *positionA, const Vector2 *scaleA,
                 const Vector2 *positionB, const Vector2 *scaleB);

// small helper — got tired of writing fmaxf(fminf(...)) everywhere
float clampf(float val, float lo, float hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

// box-to-box collision — same approach from class (lecture 3)
bool isColliding(const Vector2 *positionA, const Vector2 *scaleA,
                 const Vector2 *positionB, const Vector2 *scaleB)
{
    float xDist = fabs(positionA->x - positionB->x) 
                - ((scaleA->x + scaleB->x) / 2.0f);
    float yDist = fabs(positionA->y - positionB->y) 
                - ((scaleA->y + scaleB->y) / 2.0f);

    return (xDist < 0.0f && yDist < 0.0f);
}

// uses DrawTexturePro with originOffset so the position is the centre of the object
void renderObject(const Texture2D *texture, const Vector2 *position,
                  const Vector2 *scale)
{
    Rectangle textureArea = {
        0.0f, 0.0f,
        static_cast<float>(texture->width),
        static_cast<float>(texture->height)
    };

    Rectangle destinationArea = {
        position->x, position->y,
        scale->x, scale->y
    };

    // origin at centre so position = centre of sprite
    Vector2 originOffset = {
        scale->x / 2.0f,
        scale->y / 2.0f
    };

    DrawTexturePro(*texture, textureArea, destinationArea,
                   originOffset, 0.0f, WHITE);
}

void resetBalls()
{
    // picked these by hand so the balls spread out nicely
    float angles[] = { 0.35f, -0.5f, 0.7f };
    float xDirs[]  = { 1.0f, -1.0f, 1.0f };

    for (int i = 0; i < MAX_BALLS; i++)
    {
        gBallPositions[i] = ORIGIN;

        Vector2 dir = { xDirs[i], sinf(angles[i]) };
        Normalise(&dir);

        gBallVelocities[i] = { dir.x * BALL_SPEED, dir.y * BALL_SPEED };
        gBallActive[i] = (i < gActiveBallCount);
    }
}

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pong Clone - Neon Arcade");

    gPaddle1Texture = LoadTexture(PADDLE1_FP);
    gPaddle2Texture = LoadTexture(PADDLE2_FP);
    gBallTexture    = LoadTexture(BALL_FP);
    gP1WinsTexture  = LoadTexture(P1_WINS_FP);
    gP2WinsTexture  = LoadTexture(P2_WINS_FP);

    resetBalls();
    gPreviousTicks = (float) GetTime();

    SetTargetFPS(FPS);
}

void processInput()
{
    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) 
    {
        gAppStatus = TERMINATED;
        return;
    }

    if (gGameOver) return;

    // player 1 movement (W / S)
    gPaddle1Movement = 0.0f;
    if      (IsKeyDown(KEY_W)) gPaddle1Movement = -1.0f;
    else if (IsKeyDown(KEY_S)) gPaddle1Movement =  1.0f;

    // player 2 movement (arrows) — locked out when AI is on
    if (!gSinglePlayerMode)
    {
        gPaddle2Movement = 0.0f;
        if      (IsKeyDown(KEY_UP))   gPaddle2Movement = -1.0f;
        else if (IsKeyDown(KEY_DOWN)) gPaddle2Movement =  1.0f;
    }

    // t key toggles between 1-player and 2-player
    if (IsKeyPressed(KEY_T)) gSinglePlayerMode = !gSinglePlayerMode;

    // number keys to change how many balls are in play
    if (IsKeyPressed(KEY_ONE))   { gActiveBallCount = 1; resetBalls(); }
    if (IsKeyPressed(KEY_TWO))   { gActiveBallCount = 2; resetBalls(); }
    if (IsKeyPressed(KEY_THREE)) { gActiveBallCount = 3; resetBalls(); }
}

void update()
{
    if (gGameOver) return;

    float ticks     = (float) GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    float halfPaddle = gPaddleScale.y / 2.0f;
    float halfBall   = gBallScale.x / 2.0f;

    // --- paddle 1 ---
    gPaddle1Position.y += gPaddle1Movement * PADDLE_SPEED * deltaTime;
    gPaddle1Position.y = clampf(gPaddle1Position.y, halfPaddle, 
                                SCREEN_HEIGHT - halfPaddle);

    // --- paddle 2 / AI ---
    if (gSinglePlayerMode)
    {
        // not doing full prediction/trajectory — just tracking Y for now
        float targetY = gPaddle2Position.y;
        bool found = false;

        for (int i = 0; i < MAX_BALLS; i++)
        {
            if (!gBallActive[i]) continue;
            if (gBallVelocities[i].x <= 0.0f) continue; // only care about balls coming our way

            if (!found || gBallPositions[i].x > gBallPositions[0].x)
            {
                targetY = gBallPositions[i].y;
                found = true;
            }
        }

        float diff = targetY - gPaddle2Position.y;
        if (fabs(diff) > 5.0f)
            gPaddle2Movement = (diff > 0.0f) ? 1.0f : -1.0f;
        else
            gPaddle2Movement = 0.0f;
    }

    float p2Speed = gSinglePlayerMode ? AI_SPEED : PADDLE_SPEED;
    gPaddle2Position.y += gPaddle2Movement * p2Speed * deltaTime;
    gPaddle2Position.y = clampf(gPaddle2Position.y, halfPaddle,
                                SCREEN_HEIGHT - halfPaddle);

    // --- balls ---
    for (int i = 0; i < MAX_BALLS; i++)
    {
        if (!gBallActive[i]) continue;

        gBallPositions[i].x += gBallVelocities[i].x * deltaTime;
        gBallPositions[i].y += gBallVelocities[i].y * deltaTime;

        // top/bottom wall bounce
        if (gBallPositions[i].y - halfBall < 0.0f)
        {
            gBallPositions[i].y = halfBall;
            gBallVelocities[i].y = fabs(gBallVelocities[i].y);
        }
        if (gBallPositions[i].y + halfBall > SCREEN_HEIGHT)
        {
            gBallPositions[i].y = SCREEN_HEIGHT - halfBall;
            gBallVelocities[i].y = -fabs(gBallVelocities[i].y);
        }

        // paddle 1 collision (left side)
        if (isColliding(&gBallPositions[i], &gBallScale,
                        &gPaddle1Position, &gPaddleScale))
        {
            // prevent ball from sticking inside paddle on high dt
            gBallPositions[i].x = gPaddle1Position.x 
                                + gPaddleScale.x / 2.0f + halfBall;

            float hitRatio = (gBallPositions[i].y - gPaddle1Position.y) / halfPaddle;
            hitRatio = clampf(hitRatio, -1.0f, 1.0f);

            gBallVelocities[i].x = fabs(gBallVelocities[i].x);
            gBallVelocities[i].y = hitRatio * BALL_SPEED * 0.8f;

            // keep total speed constant
            float spd = GetLength(gBallVelocities[i]);
            gBallVelocities[i].x = (gBallVelocities[i].x / spd) * BALL_SPEED;
            gBallVelocities[i].y = (gBallVelocities[i].y / spd) * BALL_SPEED;
        }

        // same thing but for paddle 2
        if (isColliding(&gBallPositions[i], &gBallScale,
                        &gPaddle2Position, &gPaddleScale))
        {
            gBallPositions[i].x = gPaddle2Position.x 
                                - gPaddleScale.x / 2.0f - halfBall;

            float hit = (gBallPositions[i].y - gPaddle2Position.y) / halfPaddle;
            hit = clampf(hit, -1.0f, 1.0f);

            gBallVelocities[i].x = -fabs(gBallVelocities[i].x);
            gBallVelocities[i].y = hit * BALL_SPEED * 0.8f;

            float spd = GetLength(gBallVelocities[i]);
            gBallVelocities[i].x = (gBallVelocities[i].x / spd) * BALL_SPEED;
            gBallVelocities[i].y = (gBallVelocities[i].y / spd) * BALL_SPEED;
        }

        // ball went off the left side
        if (gBallPositions[i].x + halfBall < 0.0f)
        {
            gGameOver = true;
            gWinner = 2;
            return;
        }

        // off the right side
        if (gBallPositions[i].x - halfBall > SCREEN_WIDTH)
        {
            gGameOver = true;
            gWinner = 1;
            return;
        }
    }
}

void render()
{
    BeginDrawing();
    ClearBackground(ColorFromHex(BG_COLOUR));

    // dashed centre line
    for (int y = 0; y < SCREEN_HEIGHT; y += 24)
    {
        DrawRectangle(SCREEN_WIDTH / 2 - 1, y, 2, 12,
                      (Color){ 60, 60, 90, 255 });
    }

    renderObject(&gPaddle1Texture, &gPaddle1Position, &gPaddleScale);
    renderObject(&gPaddle2Texture, &gPaddle2Position, &gPaddleScale);

    for (int i = 0; i < MAX_BALLS; i++)
    {
        if (!gBallActive[i]) continue;
        renderObject(&gBallTexture, &gBallPositions[i], &gBallScale);
    }

    if (gGameOver)
    {
        Texture2D *winTex = (gWinner == 1) ? &gP1WinsTexture : &gP2WinsTexture;
        Vector2 msgScale = {
            static_cast<float>(winTex->width) * WIN_MSG_SCALE,
            static_cast<float>(winTex->height) * WIN_MSG_SCALE
        };
        renderObject(winTex, &ORIGIN, &msgScale);
    }

    EndDrawing();
}

void shutdown()
{
    UnloadTexture(gPaddle1Texture);
    UnloadTexture(gPaddle2Texture);
    UnloadTexture(gBallTexture);
    UnloadTexture(gP1WinsTexture);
    UnloadTexture(gP2WinsTexture);
    CloseWindow();
}

int main(void)
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        render();
    }

    shutdown();
    return 0;
}
