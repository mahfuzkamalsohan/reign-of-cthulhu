#include "raylib.h"

#define PLAYER_DRAW_SIZE 200

typedef enum LightState { RED_LIGHT = 0, GREEN_LIGHT = 1 } LightState;
typedef enum AnimationType { LOOP = 1, ONESHOT = 2 } AnimationType;
typedef enum Direction { LEFT = -1, RIGHT = 1 } Direction;

typedef struct Animation
{ 
    int first_frame;  
    int last_frame;  
    int current_frame;
    const float speed;
    float duration_left; 
    int row;
    AnimationType type;
} Animation;

typedef struct Player
{
    Vector2 position;
    Vector2 velocity;
    float speed;
    float jump_force;
    bool on_ground;
} Player;

void animation_update(Animation* self)
{
    self->duration_left -= GetFrameTime();
    if (self->duration_left <= 0.0f)
    {
        self->duration_left = self->speed;
        switch (self->type)
        {
            case LOOP:
                if (self->current_frame >= self->last_frame)
                    self->current_frame = self->first_frame;
                else
                    self->current_frame += 1;
                break;
            case ONESHOT:
                if (self->current_frame < self->last_frame)
                    self->current_frame += 1;
                break;
        }
    }
}

Rectangle animation_frame(Animation* self, int max_num_frames_per_row, int num_rows, Texture2D texture)
{
    float frame_width = (float)texture.width / max_num_frames_per_row;
    float frame_height = (float)texture.height / num_rows;
    float x = (self->current_frame % max_num_frames_per_row) * frame_width;
    float y = self->row * frame_height;
    return (Rectangle){x, y, frame_width, frame_height};
}

void select_player_animation(const Player* player, Animation* anim)
{
    if (!player->on_ground)
    {
        anim->row = 4;
        anim->first_frame = 0;
        anim->last_frame = 3;
        anim->type = LOOP;
    }
    else if (player->velocity.x != 0)
    {
        anim->row = 1;
        anim->first_frame = 0;
        anim->last_frame = 7;
        anim->type = LOOP;
    }
    else
    {
        anim->row = 0;
        anim->first_frame = 0;
        anim->last_frame = 7;
        anim->type = LOOP;
    }
}

int main ()
{
    InitWindow(1280, 800, "Sprite Animation");

    Texture2D player_texture = LoadTexture("assets/hero/hero.png");
    if (player_texture.id == 0)
    {
        CloseWindow();
        return 1;
    }

    LightState light_state = GREEN_LIGHT;
    double state_start_time = GetTime();
    bool is_dead = false;

    const int max_num_frames_per_row = 14;
    const int num_rows = 8;
    const float gravity = 2000.0f;
    const float ground_y = 500.0f;

    Animation player_animation = {
        .first_frame = 0,
        .last_frame = 7,
        .current_frame = 0,
        .speed = 0.1f,
        .duration_left = 0.1f,
        .row = 0,
        .type = LOOP
    };

    Direction player_direction = RIGHT;

    Player player = {
        .position = {100, 600},
        .velocity = {0, 0},
        .speed = 250.0f,
        .jump_force = 1200.0f,
        .on_ground = false
    };

    Vector2 last_position = player.position;

    while (!WindowShouldClose())
    {
        double elapsed = GetTime() - state_start_time;
        if (light_state == GREEN_LIGHT && elapsed >= 5.0) 
        {
            light_state = RED_LIGHT;
            state_start_time = GetTime();
            last_position = player.position;
        } 
        else if (light_state == RED_LIGHT && elapsed >= 3.0) 
        {
            light_state = GREEN_LIGHT;
            state_start_time = GetTime();
        }

        float dt = GetFrameTime();

        if (!is_dead) 
        {
            if (light_state == GREEN_LIGHT)
            {
                if (IsKeyDown(KEY_RIGHT))
                {
                    player_direction = RIGHT;
                    player.velocity.x = player.speed;
                }
                else if (IsKeyDown(KEY_LEFT))
                {
                    player_direction = LEFT;
                    player.velocity.x = -player.speed;
                }
                else
                {
                    player.velocity.x = 0;
                }

                if (IsKeyPressed(KEY_SPACE) && player.on_ground)
                {
                    player.velocity.y = -player.jump_force;
                    player.on_ground = false;
                }
            }
            // else
            // {
            //     player.velocity.x = 0;
            // }

            player.velocity.y += gravity * dt;
            player.position.x += player.velocity.x * dt;
            player.position.y += player.velocity.y * dt;

            if (player.position.y >= ground_y)
            {
                player.position.y = ground_y;
                player.velocity.y = 0;
                player.on_ground = true;
            }
            else
            {
                player.on_ground = false;
            }

            select_player_animation(&player, &player_animation);

            if (light_state == RED_LIGHT && 
                (player.position.x != last_position.x || player.position.y != last_position.y)) 
            {
                is_dead = true;
                player_animation.row = 7;
                player_animation.first_frame = 0;
                player_animation.last_frame = 13;
                player_animation.current_frame = 0;
                player_animation.type = ONESHOT;
                player_animation.duration_left = player_animation.speed;
                
            }

            animation_update(&player_animation);
        }
        else
        {
            animation_update(&player_animation);
        }

        BeginDrawing();
        if(light_state == GREEN_LIGHT)
        {
            ClearBackground(SKYBLUE);
            DrawText("GREEN LIGHT: Move and Jump!", 10, 10, 20, WHITE);
        }
        else
        {
            ClearBackground(RED);
            DrawText("RED LIGHT: Don't Move!", 10, 10, 20, WHITE);
        }

        Rectangle player_frame = animation_frame(&player_animation, max_num_frames_per_row, num_rows, player_texture);
        player_frame.width *= player_direction;
        
        DrawTexturePro(
            player_texture,
            player_frame,
            (Rectangle){player.position.x, player.position.y, PLAYER_DRAW_SIZE * player_direction, PLAYER_DRAW_SIZE},
            (Vector2){0, 0},
            0.0f,
            WHITE
        );

        

        EndDrawing();
    }

    UnloadTexture(player_texture);
    CloseWindow();
    return 0;
}
