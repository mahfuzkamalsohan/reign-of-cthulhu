#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>

#define PLAYER_DRAW_SIZE 100

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 720
#define GRAVITY 0.5f
#define JUMP_FORCE -12.0f
#define PLAYER_SPEED 5.0f
#define MAX_PLATFORMS 6
#define TOTAL_TIME 120.0f // seconds (2min game timer)

typedef enum AnimationType { LOOP = 1, ONESHOT = 2 } AnimationType;
typedef enum Direction { LEFT = -1, RIGHT = 1 } Direction;
typedef enum LightState { RED_LIGHT = 0, GREEN_LIGHT = 1 } LightState;

typedef struct Animation {
    int first_frame;
    int last_frame;
    int current_frame;
    const float speed;
    float duration_left;
    int row;
    AnimationType type;
} Animation;

typedef struct Player {
    Rectangle rect;
    float velocityY;
    bool isJumping;
    bool isAlive;
} Player;

void animation_update(Animation* self) {
    self->duration_left -= GetFrameTime();
    if (self->duration_left <= 0.0f) {
        self->duration_left = self->speed;
        switch (self->type) {
            case LOOP:
                if (self->current_frame >= self->last_frame)
                    self->current_frame = self->first_frame;
                else
                    self->current_frame += 1;
                break;
            case ONESHOT:
                if (self->current_frame < self->last_frame)
                    self->current_frame += 1;
                // Do NOT advance past last_frame
                // else: stay on last_frame
                break;
        }
    }
}

Rectangle animation_frame(Animation* self, int max_frames, int num_rows, Texture2D texture) {
    float frame_width = (float)texture.width / max_frames;
    float frame_height = (float)texture.height / num_rows;
    float x = (self->current_frame % max_frames) * frame_width;
    float y = self->row * frame_height;
    return (Rectangle){x, y, frame_width, frame_height};
}

void select_player_animation(bool moving, bool jumping, Animation* anim) {
    if (jumping) {
        anim->row = 4;
        anim->first_frame = 0;
        anim->last_frame = 3;
        anim->type = LOOP;
    } else if (moving) {
        anim->row = 1;
        anim->first_frame = 0;
        anim->last_frame = 7;
        anim->type = LOOP;
    } else {
        anim->row = 0;
        anim->first_frame = 0;
        anim->last_frame = 7;
        anim->type = LOOP;
    }
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Merged Platformer + Animation");

    Texture2D player_texture = LoadTexture("assets/hero/hero.png");
    if (player_texture.id == 0) {
        CloseWindow();
        return 1;
    }

    const int max_frames = 14;
    const int num_rows = 8;

    Animation player_anim = {
        .first_frame = 0,
        .last_frame = 7,
        .current_frame = 0,
        .speed = 0.1f,
        .duration_left = 0.1f,
        .row = 0,
        .type = LOOP
    };

    Player player = {
        // Set initial position (x=100, y=300), width and height based on sprite frame size
        .rect = {100, 300, PLAYER_DRAW_SIZE/2, PLAYER_DRAW_SIZE/2},
        .velocityY = 0,
        .isJumping = false,
        .isAlive = true
    };

    Rectangle platforms[MAX_PLATFORMS] = {
        {100, 345, 100, 80},
        {200, 280, 120, 180},
        {500, 280, 100, 80},
        {800, 200, 140, 80},
        {-50, -50, 50, 120},
        {200, 0, 200, 180}
    };

    // Setting up camera
    Camera2D camera = {0};
    camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    camera.target = (Vector2){player.rect.x + player.rect.width / 2, player.rect.y + player.rect.height / 2};
    camera.zoom = 1.0f;

    Direction direction = RIGHT;
    LightState light = GREEN_LIGHT;
    double state_start_time = GetTime();
    Vector2 last_pos = {player.rect.x, player.rect.y};
    bool is_dead = false;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        double elapsed = GetTime() - state_start_time;

        // Light state logic
        if (light == GREEN_LIGHT && elapsed >= 10.0) {
            light = RED_LIGHT;
            state_start_time = GetTime();
            last_pos.x = player.rect.x;
            last_pos.y = player.rect.y;
        } else if (light == RED_LIGHT && elapsed >= 2.0) {
            light = GREEN_LIGHT;
            state_start_time = GetTime();
            // is_dead = false; // Player stays dead after red light ends
        }

        bool moving = false;

        // Always apply gravity and update position
        player.velocityY += GRAVITY;
        player.rect.y += player.velocityY;

        // --- Always run collision detection ---
        bool onLeftWall = false;
        bool onRightWall = false;
        bool onPlatform = false;

        for (int i = 0; i < MAX_PLATFORMS; i++) {
            Rectangle plat = platforms[i];
            if (CheckCollisionRecs(player.rect, plat)) {
                float overlapLeft = (player.rect.x + player.rect.width) - plat.x;
                float overlapRight = (plat.x + plat.width) - player.rect.x;
                float overlapTop = (player.rect.y + player.rect.height) - plat.y;
                float overlapBottom = (plat.y + plat.height) - player.rect.y;

                float minX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
                float minY = (overlapTop < overlapBottom) ? overlapTop : overlapBottom;

                if (minX < minY) { // Horizontal collision
                    if (overlapLeft < overlapRight) {
                        player.rect.x -= overlapLeft;
                        onRightWall = true; // Check collision with right side of platform
                    } else {
                        player.rect.x += overlapRight;
                        onLeftWall = true;  // Check collision with left side of platform
                    }
                } else { // Vertical collision
                    if (overlapTop < overlapBottom) { // Collided with top of platform
                        player.rect.y -= overlapTop;
                        player.velocityY = 0;
                        player.isJumping = false;
                        onPlatform = true;
                    } else { // Collided with bottom of platform
                        player.rect.y += overlapBottom;
                        player.velocityY = 0;
                    }
                }
            }
        }

        if (!onPlatform) player.isJumping = true;

        // RED LIGHT: detect any move (always check, not just when alive)
        if (light == RED_LIGHT &&
            !is_dead &&
            ((player.rect.x != last_pos.x) || (player.rect.y != last_pos.y) || player.isJumping)) {
            is_dead = true;
            player_anim.row = 7;
            player_anim.first_frame = 0;
            player_anim.last_frame = 13;
            player_anim.current_frame = 0;
            player_anim.type = ONESHOT;
            player_anim.duration_left = player_anim.speed;
        }

        if (!is_dead) {
            // Movement with A and D
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
                player.rect.x -= PLAYER_SPEED;
                direction = LEFT;
                moving = true;
            }
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
                player.rect.x += PLAYER_SPEED;
                direction = RIGHT;
                moving = true;
            }

            // Jump with W or SPACE
            if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) && light == GREEN_LIGHT) {
                if (!player.isJumping && onPlatform) {
                    player.velocityY = JUMP_FORCE;
                    player.isJumping = true;
                } else if (onLeftWall || onRightWall) {
                    player.velocityY = JUMP_FORCE;
                    player.isJumping = true;
                    if (onLeftWall) player.rect.x += 25.0f;
                    if (onRightWall) player.rect.x -= 25.0f;
                }
            }

            // Force idle animation if standing on platform and not moving/jumping
            if (onPlatform && !moving && !player.isJumping) {
                player_anim.row = 0;
                player_anim.first_frame = 0;
                player_anim.last_frame = 7;
                player_anim.type = LOOP;
            } else {
                select_player_animation(moving, player.isJumping, &player_anim);
            }
            animation_update(&player_anim);
        } else {
            // Only update animation if dead
            animation_update(&player_anim);
        }

        // Update camera to follow player
        camera.target = (Vector2){player.rect.x + player.rect.width / 2, player.rect.y + player.rect.height / 2};

        BeginDrawing();
        ClearBackground(light == GREEN_LIGHT ? SKYBLUE : RED);

        BeginMode2D(camera);

        // Draw platforms
        for (int i = 0; i < MAX_PLATFORMS; i++) {
            DrawRectangleRec(platforms[i], DARKGRAY);
        }

        Rectangle frame = animation_frame(&player_anim, max_frames, num_rows, player_texture);
        frame.width *= direction;

        DrawTexturePro(
            player_texture,
            frame,
            (Rectangle){
                player.rect.x + player.rect.width / 2 - PLAYER_DRAW_SIZE / 2,
                player.rect.y + player.rect.height / 2 - PLAYER_DRAW_SIZE / 2,
                PLAYER_DRAW_SIZE * direction,
                PLAYER_DRAW_SIZE
            },
            (Vector2){0, 0},
            0.0f,
            WHITE
        );

        // Visualize the collision box
        DrawRectangleLines(
            (int)player.rect.x,
            (int)player.rect.y,
            (int)player.rect.width,
            (int)player.rect.height,
            GREEN  // Pick any color
        );

        EndMode2D();

        DrawText(light == GREEN_LIGHT ? "GREEN LIGHT: MOVE!" : "RED LIGHT: DON'T MOVE!", 20, 20, 20, WHITE);

        EndDrawing();
    }

    UnloadTexture(player_texture);
    CloseWindow();
    return 0;
}
