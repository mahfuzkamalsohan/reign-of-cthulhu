#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define PLAYER_DRAW_SIZE 100
#define MOB_DRAW_SIZE 350 

#define MAX_DOTS 3
#define DOT_RADIUS 20
#define NOCLIP 1
#define GRAVITY 0.5f
#define JUMP_FORCE -10.0f
#define PLAYER_SPEED 5.0f
#define MAX_PLATFORMS 50
#define DOUBLE_JUMPS 5
#define DASHES 2
#define TOTAL_TIME 120.0f // seconds (2min game timer)
#define DASH_DISTANCE 300.0f
#define DASH_STEP 60.0f
#define EYEBALL_MOB_TIMER 1.0f // seconds
#define CheckPointcount 10
#define LEVITATION 1
#define LEVITATION_TIMER 5.0f
#define NOCLIP_TIMER 4.0f




typedef enum AnimationType { LOOP = 1, ONESHOT = 2 } AnimationType;
typedef enum Direction { LEFT = -1, RIGHT = 1 } Direction;
typedef enum LightState { RED_LIGHT = 0, GREEN_LIGHT = 1 } LightState;
typedef enum TILE_TYPE { TOP_TILE = 0, LEFT_TILE = 1, RIGHT_TILE = 2, BOTTOM_TILE = 3, CENTER_TILE = 4, LEFT_TOP_TILE = 5, RIGHT_TOP_TILE = 6, LEFT_BOTTOM_TILE = 7, RIGHT_BOTTOM_TILE = 8, TOP_LEFT_ANGLE = 9, RIGHT_BOTTOM_ANGLE = 10} TILE_TYPE;

typedef struct Animation {
    int first_frame;
    int last_frame;
    int current_frame;
    float speed;
    float duration_left;
    int row;
    AnimationType type;
} Animation;

typedef struct Player {
    Rectangle rect;     //player hitbox
    float velocityY;    //vertical velocity
    int facingDirection;    //direction in which player is facing //-1 for left (subtracting abscissa), +1 for right
    int gravitySign;

    bool phaseActive;     
     

    bool isAlive;       //alive status

    int health;         //health of player

    bool isJumping;     //if player is on air 
    bool isDashing;     //if player is dashing
    bool isWallSliding;      
    bool laserAcquired;
    int doubleJumpCount;    //consumable double jumps

    int dashCount;      //consumable dashes
    float dashTargetX;  //
    int dashFrames;
    bool isAttacking; 
    bool isDealingDamage;
} Player;

typedef struct {
    Vector2 pos;
    bool active;
    float timer;
} Dot;

typedef struct Mob {
    Rectangle collider;
    Rectangle hitbox; //mob hitbox
    int mobHealth;
    bool isAlive;
    bool isActive; 
    float timer;
} Mob;

typedef struct PowUpDjump {
    Rectangle rect;
    bool isCollected;
} PowUpDjump;

typedef struct PowUpDash {
    Rectangle rect;
    bool isCollected;
} PowUpDash;

typedef struct PowUpNoclip {
    Rectangle rect;
    bool isCollected;
} PowUpNoclip;

typedef struct PowUpLevitation {
    Rectangle rect;
    bool isCollected;
} PowUpLevitation;

//BRAIN DEFINE
typedef struct Brain {
    Rectangle position;    // used for collisions
    float radius;      // for drawing
    int brainHealth;
    bool isAlive;

    float speedX;      // horizontal speed
    bool dropping;
    float floatY;      // resting Y for hover
    bool goingUp;
    
} Brain;


void DrawTilemap(Texture2D tileset, int rows, int cols, int tilemap[rows][cols], int startX, int startY, int tileSize) {
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int tileIndex = tilemap[y][x];

            Rectangle src = {
                tileIndex * tileSize,  	
                0,
                tileSize,
                tileSize
            };

            Rectangle dest = {
                startX + x * tileSize,
                startY + y * tileSize,
                tileSize,
                tileSize
            };

            DrawTexturePro(tileset, src, dest, (Vector2){0, 0}, 0, WHITE);
        }
    }
}


void boss_awake_animation(Animation* anim) {
    anim->row = 1;
    anim->first_frame = 0;
    anim->last_frame = 3;
    anim->current_frame = 0;
    anim->type = ONESHOT;
    anim->duration_left = anim->speed;
}

void boss_sleep_animation(Animation* anim) {
    anim->row = 0;
    anim->first_frame = 0;
    anim->last_frame = 3;
    anim->current_frame = 0;
    anim->type = ONESHOT;
    anim->duration_left = anim->speed;
}

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
    int prev_row = anim->row;
    AnimationType prev_type = anim->type;

    if (jumping) {
        anim->first_frame = 0;
        anim->row = 4;
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

    // Reset current_frame if animation row or type changed
    if (anim->row != prev_row || anim->type != prev_type) {
        anim->current_frame = 0;
        anim->duration_left = anim->speed;
    }
}


//=============================== Animation Functions ===================================//

void death_animation(Animation* anim) {
    anim->row = 7;
    anim->first_frame = 0;
    anim->last_frame = 13;
    anim->current_frame = 0;
    anim->type = ONESHOT;
    anim->duration_left = anim->speed;
}

void player_attack_animation1(Animation* anim) {
    anim->row = 2;
    anim->first_frame = 0;
    anim->last_frame = 3;
    anim->current_frame = 0;
    anim->type = ONESHOT;
    anim->duration_left = anim->speed;

}
void player_attack_animation2(Animation* anim) {
    anim->row = 3;
    anim->first_frame = 0;
    anim->last_frame = 2;
    anim->current_frame = 0;
    anim->type = ONESHOT;
    anim->duration_left = anim->speed;
}
void idle_animation(Animation* anim) {
    
    anim->row = 0;
    anim->first_frame = 0;
    anim->last_frame = 7;
    anim->current_frame = 0;
    anim->type = LOOP;
    anim->duration_left = anim->speed;
    
}

void hit_animation(Animation* anim) {
    anim->row = 6;
    anim->first_frame = 1;
    anim->last_frame = 1;
    anim->current_frame = 1;
    anim->type = ONESHOT;
    anim->duration_left = anim->speed;
}
void mob_attack_animation1(Animation* anim) {
    anim->row = 3;
    anim->first_frame = 0;
    anim->last_frame = 9;
    anim->current_frame = 0;
    anim->type = ONESHOT;
    anim->duration_left = anim->speed;
}
void mob_attack_animation2(Animation* anim) {
    anim->row = 3;
    anim->first_frame = 0;
    anim->last_frame = 9;
    anim->current_frame = 0;
    anim->type = ONESHOT;
    anim->duration_left = anim->speed;
}
void mob_idle_animation(Animation* anim) {
    anim->row = 0;
    anim->first_frame = 0;
    anim->last_frame = 9;
    anim->current_frame = 0;
    anim->type = LOOP;
    anim->duration_left = anim->speed;
}




int main() {
    int screenWidth = 1080;
    int screenHeight = 720;

    const int virtualWidth = 1080;
    const int virtualHeight = 720;


    int cameraMode = 0; // 0 for dynamic, 1 for static
    int worldMode = 0; // 0 for overworld, 1 for boss arena
    Vector2 spawnPoint = { 100, 100 }; 

    // Vector2 dotPos = {-100,-100};
    // bool dotActive = false;
    // float dotTimer = 0;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);

    InitAudioDevice();                    
    Sound awake_fx = LoadSound("assets/sound/awake_fx.mp3");  
    bool soundPlayed = false; 


    
    InitWindow(screenWidth, screenHeight, "Merged Platformer + Animation");
    RenderTexture2D target = LoadRenderTexture(virtualWidth, virtualHeight);
    const int TILE_SIZE = 32;  //each tile is 32x32 px

    Texture2D tileset = LoadTexture("assets/map/tileset.png");
    Texture2D teleporter_texture = LoadTexture("assets/map/teleporter.png");
    Texture2D wizard_texture = LoadTexture("assets/npc/wizard.png");
    Texture2D laser_texture = LoadTexture("assets/hero/laser.png");
    Texture2D brain_texture = LoadTexture("assets/boss/brain.png");
    Texture2D boss_arena_tileset = LoadTexture("assets/map/boss_arena_tileset.png");
    Texture2D boss_arena_background = LoadTexture("assets/map/boss_arena.png");
    Texture2D player_texture = LoadTexture("assets/hero/hero.png");
    Texture2D mob_texture = LoadTexture("assets/enemies/eyehead.png");
    Texture2D boss_background = LoadTexture("assets/boss/boss_static.png");
    Texture2D boss_awake_texture = LoadTexture("assets/boss/boss_awake.png");
    Texture2D boss_sleep_texture = LoadTexture("assets/boss/boss_sleep.png");
    Texture2D double_jump_texture = LoadTexture("assets/powerups/djump.png");
    Texture2D dash_texture = LoadTexture("assets/powerups/dash.png");
    Texture2D eyeball = LoadTexture("assets/enemies/eyeball.png");
    Texture2D levitation_texture = LoadTexture("assets/powerups/levitation.png");


    if (player_texture.id == 0 || mob_texture.id == 0) {
        CloseWindow();
        return 1;
    }

    const int max_frames = 14;
    const int num_rows = 8;

    Animation player_anim = {
        .first_frame = 0,
        .last_frame = 7,
        .current_frame = 0,
        .speed = 0.15f,
        .duration_left = 0.1f,
        .row = 0,
        .type = LOOP
    };

    Animation boss_anim = {
        .first_frame = 0,
        .last_frame = 3,
        .current_frame = 0,
        .speed = 0.15f,
        .duration_left = 0.1f,
        .row = 0,
        .type = ONESHOT
    };

    const int mob_max_frames = 10;
    const int mob_num_rows = 6;

    const int boss_max_frames = 4;
    const int boss_num_rows = 1;

    Animation mob_anim = {
        .first_frame = 0,
        .last_frame = 9,
        .current_frame = 0,
        .speed = 0.1f,
        .duration_left = 0.1f,
        .row = 0,
        .type = LOOP
    };

//======================================Player Initialization=================================//

    Player player = {
        //player collision box x, y, width, height
        .rect = {spawnPoint.x,spawnPoint.y, PLAYER_DRAW_SIZE/4, PLAYER_DRAW_SIZE/2},
        .velocityY = 0,
        .facingDirection = 1,
        .gravitySign = 1,
        .phaseActive = false,
        
        .isAlive = true,

        .health = 3,

        .isJumping = false,

        .doubleJumpCount = 0,
        .laserAcquired = false,
        .dashCount = 0,
        .isDashing = false,
        .isAttacking = false,
        .isDealingDamage = false
    };


    //LASER INITIALIZE
    bool laserActive = false;
    Rectangle laserRect = {0};   // laser hitbox
    float laserLength = 800;     // how long the laser reaches
    float laserDuration = 0.2f;  // how long laser stays on screen in seconds
    float laserTimer = 0.0f;

    Mob mob = {
        .collider = {400, 50, 200, 50},
        .hitbox = {400, 50, 20, 50},
        .mobHealth = 3,
        .isAlive = true,
        .isActive = false,
        .timer = EYEBALL_MOB_TIMER
    };



    //BRAIN INITIALIZE
    Brain brain = {
        .position = {20000+512, -20256, 200, 200}, // collision rectangle
        .radius = 100,                         // circle radius
        .brainHealth = 100,
        .isAlive = true,
        .speedX = 2.0f,
        .dropping = false,
        .floatY = -20256,
        .goingUp = false
    };

    Dot dots[MAX_DOTS];

    for (int i = 0; i < MAX_DOTS; i++) {
    dots[i].active = false;
    dots[i].timer = 0;
    dots[i].pos = (Vector2){0, 0};
}

    





   //======================================Level Layout PLATFORMS=========================================//

    //add platforms with physics
    // Rectangle platforms[MAX_PLATFORMS] = {
    //     {100, 345, 160, 160},
    //     {200, 280, 128, 192},
    //     {400, 100, 192, 192}
    // };
    // int platform1[5][5] = {
	// 	{LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE},
	// 	{LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
	// 	{LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
	// 	{LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
	// 	{LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
	// };

	// int platform2[6][4] = {
	// 	{LEFT_TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
	// 	{LEFT_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
	// 	{TOP_LEFT_ANGLE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    //     {CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    //     {CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    //     {CENTER_TILE, RIGHT_BOTTOM_ANGLE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
	// };

    // int platform3[6][6] = {
    //     {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    //     {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    //     {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    //     {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    //     {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    //     {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    // };
    Rectangle teleportZoneA = { 4700, 60, 576, 256};
    Rectangle teleportZoneB = { 4800, 500, 160, 64 };
    Rectangle Checkpoint[CheckPointcount] = 
    {
        {1450,100,32,32},
        {2550,50,32,32},
        {4150,50,32,32},
        {5830,130,32,32},
        {8130, 200,32,32},
        {9730, 100,32,32}
    };

    

    Rectangle platforms[MAX_PLATFORMS] = {
        {100, 345, 160, 160},
        {200, 280, 128, 192},
        {400, 100, 192, 192},
        {700, 150, 256, 128},
        {900, 250, 256, 128},
        {1200, 260, 160, 160}, 
        {1400, 100, 192, 160},
        {1700, 200, 160, 160}, 
        {1860, 300, 160, 160},
        {2100, 250, 192, 128},  
        {2300, 200, 160, 128},   
        {2500, 50, 256, 128},   
        {2800, 150, 192, 128},    
        {3150, 180, 192, 160},   
        {3350, 100, 160, 128},  
        {3500, 140, 128, 96},    
        {3700, 220, 192, 96},     
        {3800, 100, 224, 128}, 
        {4100, 50, 160, 160},   
        {4400, 90, 192, 128},    
        {4700, 60, 576, 256},    
        {5400, 310, 320, 224},   
        {5800, 130, 256, 160},//New From here->
        {4800, 500, 160, 64},
        {6100, 400, 192, 128},
        {6400, 300, 160, 128},
        {6700, 400, 160, 96},
        {7000, 150, 256, 128},
        {7170, 190, 160, 96},
        {7500, 350, 256, 128},
        {7670, 280, 256,128},
        {8100, 200, 192, 128},
        {8400, 100, 160, 128},
        {8700, 250, 256, 128},
        {8900, 300, 160, 96},
        {9300, 200, 192, 128},
        {9700, 100, 224, 128},
        {10100, 250, 256, 128},
        {10300, 160, 192, 128},

        //BOSS ARENA WALLS

        {20000, -20000, 1024, 1024},            // bottom wall
        //left wall
        {20000-1024, -20000-1024, 1024,1024},            // left wall
        //right wall
        {20000+1024, -20000-1024, 1024,1024},          // right wall

        {20000+928, -20000-200, 96, 96}, //right small platform 1

        {20000+928-300, -20000-200-150, 128, 96}, //right small platform 2

        {20000, -20000-200, 96, 96}, //left small platform 1

        {20000+250, -20000-200-150, 128, 96} //left small platform 2






    };

    




    int platform1[5][5] = {
		{LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE},
		{LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
		{LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
		{LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
		{LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
	};

	int platform2[6][4] = {
		{LEFT_TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
		{LEFT_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
		{LEFT_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {CENTER_TILE, CENTER_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
	};

    int platform3[6][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform4[4][8] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE ,CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, CENTER_TILE}
    };

    int platform5[4][8] = {
        {CENTER_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE ,CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform6[5][5] = {
		{LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
		{LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
		{LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
		{LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
		{LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
	};

    int platform7[5][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform8[5][5] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform9[5][5] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    
    int platform10[5][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform11[4][5] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform12[4][8] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform13[5][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };



    int platform14[5][5] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform15[4][5] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform16[3][4] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform17[3][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, CENTER_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform18[4][7] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, CENTER_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform19[5][5] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform20[4][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform21[8][18] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform22[7][10] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    int platform23[5][8] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };


    int platform24[2][5] ={
        {LEFT_TOP_TILE, TOP_TILE ,TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE ,BOTTOM_TILE,BOTTOM_TILE,RIGHT_BOTTOM_TILE}
    };
    int platform25[4][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform26[3][4] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform27[3][5] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform28[4][8] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform29[3][5] = {
        
        {CENTER_TILE, CENTER_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform30[4][8] = {
    {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform31[4][8] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {CENTER_TILE, CENTER_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform32[4][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform33[4][5] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform34[4][8] = {
    {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform35[3][5] = {
        {CENTER_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform36[4][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform37[4][7] = {
    {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform38[4][8] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, CENTER_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
    int platform39[4][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {   CENTER_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };



    //BOSS ARENA WALLS ARRAYS
    int platform40[32][32] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    //left wall
      int platform41[32][32] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        // Fill all rows with CENTER_TILE except first and last
        // Each row must have 32 columns
        // Left wall, 30 center, right wall
        
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    //right wall
      int platform42[32][32] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        // Fill all rows with CENTER_TILE except first and last
        // Each row must have 32 columns
        // Left wall, 30 center, right wall
        
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };


        int platform43[3][3] = {
        {LEFT_TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };
        int platform44[3][4] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };




    //temporary damage block
    Rectangle damageBlock = { 300, -2000, 50, 50 };






    //add power ups
    PowUpDjump Djumps[DOUBLE_JUMPS] = {
    //    { {100, 0, 20, 20}, false },
       { {100,120,32,32}, false },
       { {4850, 450, 32, 32}, false },

       { {6810, 250, 32, 32}, false }
    };

    PowUpDash Dashes[DASHES] = {
        { {300,-40,32,32}, false } , 
        { {6800, 300, 32, 32}, false }      
    };

    PowUpNoclip Noclips[NOCLIP] = {
        {{200,-40, 32,32 }, false}
    };

    PowUpLevitation Levitations[LEVITATION] = {
       { {400, -70, 32, 32}, false }
    };

    

    float gravityTimer = 0.0f;
    float phaseTimer = 0.0f;

    Rectangle playButton = { 475, 350, 100, 50 };
    Rectangle playButton2 = {475, 450,100,50};
    bool gamestarted = false;

    Rectangle quitbutton = {475, 550, 100, 50};



    bool showDialogue = false;
    const char* dialogueText1 = "Hello, wanderer!  if you wish to slay the beast, \nyou must use this ability i am giving you\nI stole this spell from the temple of SHOGGOTH. \n\nPress the 'E' key to use the ability. \nGood luck! You need it! Ohohoho!";
    const char* dialogueText2 = "Go now, and may fortune favor you!";
    Rectangle dialogueRange = { 5800, 180, 128, 192 };  // Same as destRec from earlier




//====================================Camera Setting========================================//


    // Setting up camera
    Camera2D camera = {0};
    camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
    camera.target = (Vector2){player.rect.x + player.rect.width / 2, player.rect.y + player.rect.height / 2};
    camera.zoom = 1.0f;

    Direction direction = RIGHT;
    LightState light = GREEN_LIGHT;
    double state_start_time = GetTime();
    Vector2 last_pos = {player.rect.x, player.rect.y};

    //BOSS ARENA SPAWN POINT
    Vector2 bossArenaSpawn = {20000+512, -20000-256};  // center of arena
   
    int last_anim_row = -1;

//===================================main game loop=======================================//

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsWindowResized() && !IsWindowFullscreen())
        {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
        }
        
        // check for alt + enter
 		if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
 		{
            // see what display we are on right now
 			int display = GetCurrentMonitor();
 
            
            if (IsWindowFullscreen())
            {
                // if we are full screen, then go back to the windowed size
                SetWindowSize(screenWidth, screenHeight);
            }
            else
            {
                // if we are not full screen, set the window size to match the monitor we are on
                SetWindowSize(GetMonitorWidth(display), GetMonitorHeight(display));
            }
 
            // toggle the state
 			ToggleFullscreen();
        }

        float dt = GetFrameTime();
        double elapsed = GetTime() - state_start_time;

        Vector2 mousePoint = GetMousePosition();

        if(!gamestarted){

        if (CheckCollisionPointRec(mousePoint, playButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            gamestarted = !gamestarted; 
        }
        if (CheckCollisionPointRec(mousePoint, playButton2) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            gamestarted = !gamestarted;
            player.rect.x = bossArenaSpawn.x;
                player.rect.y = bossArenaSpawn.y;
                cameraMode = 1; //static camera
                worldMode = 1; //boss arena background
                
        }
        if (CheckCollisionPointRec(mousePoint, quitbutton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            return 0;
        }
    }
        
        if (player.isAlive && gamestarted) {


            if(worldMode == 1) // if in boss arena
            {
                //BRAIN MOVEMENT
                if (brain.brainHealth > 0)
                {

                    // horizontal movement
                    if (brain.brainHealth > 0) 
                    {

                       

                        // HORIZONTAL movement always
                        brain.position.x += brain.speedX;

                        // reverse direction at horizontal bounds
                        if (brain.position.x <= 20000) brain.speedX = 2.0f;
                        if (brain.position.x + brain.position.width >= 21024) brain.speedX = -2.0f;

                        // occasionally start dropping (only when not already dropping)
                        if (!brain.dropping && !brain.goingUp && GetRandomValue(0, 100) > 98) {
                            brain.dropping = true;
                        }

                        // VERTICAL movement
                        if (brain.dropping) {
                            brain.position.y += 4;  // drop speed
                            if (brain.position.y >= -20200) { // bottom Y
                                brain.dropping = false;
                                brain.goingUp = true;       // start floating back up
                            }
                        }
                        else if (brain.goingUp) {
                            brain.position.y -= 2;          // float up speed
                            if (brain.position.y <= -20620) { // float Y
                                brain.position.y = -20620;
                                brain.goingUp = false;
                            }
                        }
                    
                
                    
                    }

                    // Check collision with player
                    Vector2 brainCenter = {
                        brain.position.x + brain.position.width / 2,
                        brain.position.y + brain.position.height / 2
                    };
                    float brainRadius = brain.position.width / 2;

                    if (CheckCollisionCircleRec(brainCenter, brainRadius, player.rect)) {
                        player.health--;   // player takes damage
                        
                        if (player.health == 0) {
                    
                            player.isAlive = false;
                            hit_animation(&player_anim);
                            death_animation(&player_anim);
                            
                        } 
                        else {
                            player.rect.x -= 40; // knockback
                        }
                    }
                     //check for player attack
                    if (player.isDealingDamage && brain.isAlive) {
                    //create an attack box larger than player hitbox
                        Rectangle attackBox = player.rect;
                        attackBox.width += 20.00f;


                    // Check collision with brain
                        if (CheckCollisionCircleRec(brainCenter, brainRadius, attackBox)) {
                            if (brain.brainHealth > 0) brain.brainHealth--; // decrease brain health
                        }
                        if(brain.brainHealth == 0){
                            brain.isAlive = false;
                    
                        }
                    }


                   

                }
            }


           

                
                // Light state logic (comment this if GREEN LIGHT AT ALL TIMES NEEDED)

                //==========================================================================================
            // if(worldMode == 0) //if in overworld use RED LIGHT/GREEN LIGHT CYCLE
            // {  
            //     if (light == GREEN_LIGHT) {
            //         if (elapsed >= 9.0 && !soundPlayed) {   // Play sound 1 second before red light
            //             PlaySound(awake_fx);
            //             soundPlayed = true;
            //         }
            //         if (elapsed >= 10.0) {
            //             boss_awake_animation(&boss_anim);
            //             light = RED_LIGHT;
            //             state_start_time = GetTime();
            //             soundPlayed = false;   // reset for next cycle
            //         }
            //     } else if (light == RED_LIGHT && elapsed >= 5.0) {
            //         boss_sleep_animation(&boss_anim);
            //         light = GREEN_LIGHT;
            //         state_start_time = GetTime();
            //         soundPlayed = false;   // reset for next cycle
            //     }
            // }




            //==========================================================================================


            
            
        //     if (light == GREEN_LIGHT) {
        //     if (elapsed >= 9.0 && !soundPlayed) {   
        //         PlaySound(awake_fx);
        //         soundPlayed = true;
        //     }
        //     if (elapsed >= 10.0) {
        //         boss_awake_animation(&boss_anim);
        //         light = RED_LIGHT;
        //         state_start_time = GetTime();
        //         soundPlayed = false;   // reset for next cycle
        //     }
        // } else if (light == RED_LIGHT) {
        //     if (elapsed >= 5.0) {
        //         boss_sleep_animation(&boss_anim);
        //         light = GREEN_LIGHT;
        //         state_start_time = GetTime();
        //         soundPlayed = false;   // reset for next cycle
        //     }
        // }q
        


            bool moving = false;  // Track if player is moving for animation
            

            //=================================Power Up Section===================================//

            //check if double jump powerup collected
            for (int i = 0; i < DOUBLE_JUMPS; i++) {
                if (CheckCollisionRecs(player.rect, Djumps[i].rect) && Djumps[i].isCollected == false) {
                    Djumps[i].isCollected = true;
                    (player.doubleJumpCount)++;
                }
            }

            //check if dash powerup collected
            for (int i = 0; i < DASHES; i++) {
                if (CheckCollisionRecs(player.rect, Dashes[i].rect) && Dashes[i].isCollected == false) {
                    Dashes[i].isCollected = true;
                    (player.dashCount)++;
                }
            }

            for (int i=0 ; i<LEVITATION; i++){
                    if (CheckCollisionRecs(player.rect, Levitations[i].rect) && Levitations[i].isCollected == false) {
                    Levitations[i].isCollected = true;
                    player.gravitySign = -0.25f;
                    gravityTimer = LEVITATION_TIMER;

                }
            }

            if (gravityTimer > 0.0f) {
                gravityTimer -= GetFrameTime();

                if (gravityTimer <= 0.0f) {
                player.gravitySign = 1.0f;   // reset to normal gravity
                gravityTimer = 0.0f;
                }
                }

            for (int i = 0; i < NOCLIP; i++) {
        if (CheckCollisionRecs(player.rect, Noclips[i].rect) && !Noclips[i].isCollected) {
            Noclips[i].isCollected = true;
            player.phaseActive = true;       
            phaseTimer = NOCLIP_TIMER;       
        }
        }
        if (phaseTimer > 0.0f) {
        phaseTimer -= GetFrameTime();

        if (phaseTimer <= 0.0f) {
        player.phaseActive = false;   // turn off noclip
        phaseTimer = 0.0f;
        }
        }       



            //============================Collision part===========================================//

            // Detect collisions for jump
            bool onLeftWall = false;    //touching left side of wall
            bool onRightWall = false;   //touching right side of wall
            bool onPlatform = false;    //standing on platform

            float wallMargin = 2.0f;

            // for (int i = 0; i < MAX_PLATFORMS; i++) {
            //     Rectangle plat = platforms[i];  //check each platform one by one

            //     if (CheckCollisionRecs(player.rect, plat)) {
            //         // Detect right side wall contact
            //         if (fabs((player.rect.x + player.rect.width) - plat.x) < wallMargin &&
            //             player.rect.y + player.rect.height > plat.y &&
            //             player.rect.y < plat.y + plat.height) {
            //                 onRightWall = true;
            //         }

            //         // Detect left side wall contact
            //         if (fabs(player.rect.x - (plat.x + plat.width)) < wallMargin &&
            //              player.rect.y + player.rect.height > plat.y &&
            //             player.rect.y < plat.y + plat.height) {
            //                 onLeftWall = true;
            //         }
            //         // Calculate how much the player overlaps the platform
            //         float overlapLeft = (player.rect.x + player.rect.width) - plat.x;
            //         float overlapRight = (plat.x + plat.width) - player.rect.x;
            //         float overlapTop = (player.rect.y + player.rect.height) - plat.y;
            //         float overlapBottom = (plat.y + plat.height) - player.rect.y;

            //         // Find smallest overlap to determine collision direction
            //         float minOverlapX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
            //         float minOverlapY = (overlapTop < overlapBottom) ? overlapTop : overlapBottom;

            //         if (minOverlapX < minOverlapY) {            // Horizontal collision
            //             if (overlapLeft < overlapRight) {       //collision from left side
            //                 player.rect.x -= overlapLeft;       //move player position to negate overlap
            //             } else {                                //collision from right side
            //                 player.rect.x += overlapRight;
            //             }
            //         } else {                                    // Vertical collision
            //             if (overlapTop < overlapBottom) {       // Collided with top of platform
            //                 player.rect.y -= overlapTop;
            //                 player.velocityY = 0;
            //                 player.isJumping = false;
            //                 onPlatform = true;
            //             } else {                                // Collided with bottom of platform
            //                 player.rect.y += overlapBottom;
            //                 player.velocityY = 0;
            //             }
            //         }
            //     }
            // }

            for (int i = 0; i < MAX_PLATFORMS; i++) {
               
                Rectangle plat = platforms[i];

                // Wall contact check (Touches platform wall, not overlap i.g - no collision)
                // Left wall detection (player touching left side of platform)
                if ((player.rect.x + player.rect.width >= plat.x - wallMargin) &&   //player width overlaps with left side of platform + margin
                    (player.rect.x + player.rect.width <= plat.x + wallMargin) &&   //player width does not overlap too deep from left side of platform
                                                                                    //range margin distance outside -> platform wall -> margine distance inside
                    (player.rect.y + player.rect.height > plat.y) &&                //player body under top of platform
                    (player.rect.y < plat.y + plat.height)){                        //player body under bottom of platform
                        onLeftWall = true;
                }

                // Right wall detection (player touching right side of platform)
                if ((player.rect.x <= plat.x + plat.width + wallMargin) &&
                    (player.rect.x >= plat.x + plat.width - wallMargin) &&
                    (player.rect.y + player.rect.height > plat.y) &&
                    (player.rect.y < plat.y + plat.height)) {
                        onRightWall = true;
                }


                // Collision Response 
                if (CheckCollisionRecs(player.rect, plat)) {        //check for collision
                    float overlapLeft = (player.rect.x + player.rect.width) - plat.x;   //overlap from left side
                    float overlapRight = (plat.x + plat.width) - player.rect.x;         //overlap from right side   
                    float overlapTop = (player.rect.y + player.rect.height) - plat.y;   //overlap from top
                    float overlapBottom = (plat.y + plat.height) - player.rect.y;       //overlap from bottom

                    float minOverlapX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;  //checks which overlap less between left and right
                    float minOverlapY = (overlapTop < overlapBottom) ? overlapTop : overlapBottom;  //checks which overlap less between top and bottom

                    if (minOverlapX < minOverlapY) {            //if horizontal overlap less
                        if(!player.phaseActive){
                        if (overlapLeft < overlapRight) {       //if left overlap less, move to left of platform 
                            player.rect.x -= overlapLeft;
                        }
                        else {
                            player.rect.x += overlapRight;
                        }
                    }
                    }
                    else{
                        if (overlapTop < overlapBottom) {
                            player.rect.y -= overlapTop;
                            player.velocityY = 0;
                            player.isJumping = false;
                            onPlatform = true;
                        }
                        else {
                            player.rect.y += overlapBottom;
                            player.velocityY = 0;
                        }
                    }
                }
            }

            if (!onPlatform) player.isJumping = true;           // If not on a platform, player is jumping



            //============================== Dialogue Section ================================//
            if (player.rect.x > 5800 && player.rect.x < 6000) 
                {
                // Player is in NPC range
                if (IsKeyPressed(KEY_UP)) {
                    showDialogue = true;
                    player.laserAcquired = true;
                }
            } 
            else 
            {
                showDialogue = false; // Player left the range
            }

            


            
            

            //======================================Movement Section/Controls=======================================//

            if ((onLeftWall || onRightWall) && !onPlatform) {        //wallslide if player touching right/left wall
                if((onLeftWall && (player.facingDirection == 1)) || (onRightWall && (player.facingDirection == -1))){
                player.isWallSliding = true;
                player.isJumping = true;
                }
            }
            
            if (((player.isWallSliding && IsKeyPressed(KEY_S))|| onPlatform || !(onLeftWall || onRightWall))) {
                player.isWallSliding = false;
                 // Add horizontal push-off from wall
                    if (onRightWall) player.rect.x += 3.0f;  // push right
                    if (onLeftWall) player.rect.x -= 3.0f; // push left
            }


            // Movement with A and D
                if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
                    player.facingDirection = -1;
                    if(!player.isWallSliding){          //disable horizontal movement during wallslide
                        player.rect.x -= PLAYER_SPEED;
                    }
                    direction = LEFT;
                    moving = true;
                }
                if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
                    player.facingDirection = 1;
                    if(!player.isWallSliding){          //disable horizontal movement during wallslide
                        player.rect.x += PLAYER_SPEED;
                    }
                    direction = RIGHT;
                    moving = true;
                }
            

            
            // Dash with LShift
            if (IsKeyPressed(KEY_LEFT_SHIFT) && player.dashCount > 0 && !player.isDashing) {  //dash count = consumable dash, dont dash if one dash already happening
                player.isDashing = true;            //toggle dashing status on

                player.dashTargetX = player.rect.x + player.facingDirection * DASH_DISTANCE;    //position at end of dash
                player.dashFrames = (int)(DASH_DISTANCE / DASH_STEP);    //how many frames dash last, increase dash step to make dash faster
                player.dashCount--; //consume dash count
            }
            if (player.isDashing) {         //iteration for each dash frame
                float step = DASH_STEP * player.facingDirection;
                
                Rectangle next_rect = player.rect; //placeholder rectangle to check for collisions before changing player pos
                next_rect.x = player.rect.x + step; //updating position of rectangle to next frame of dash

                bool collision = false;
                for (int i = 0; i < MAX_PLATFORMS; i++) {       //checking collision with platforms
                    if (CheckCollisionRecs(next_rect, platforms[i])) {
                        
                        if (player.facingDirection == 1) {
                            player.rect.x = platforms[i].x - player.rect.width;     //move player to wall side if there is collision
                        }
                        else {
                            player.rect.x = platforms[i].x + platforms[i].width;
                        }
                        collision = true;
                        break;
                    }
                }

                if (collision ||
                    (player.facingDirection == 1 && player.rect.x >= player.dashTargetX) ||
                    (player.facingDirection == -1 && player.rect.x <= player.dashTargetX)) {
                        player.isDashing = false;   //if collides, stop dash
                        player.dashFrames = 0;      //reset dash frames
                }
                else {
                    player.rect.x = next_rect.x;
                    player.dashFrames--;        //change position and reduce frame if no collision
                    if (player.dashFrames <= 0) {
                        player.isDashing = false;   //stop dash if dash frame runs out
                    }
                }
            }


            //BOSS ARENA TELEPORT
            // Debug teleport key
            if (IsKeyPressed(KEY_T)) {
                player.rect.x = bossArenaSpawn.x;
                player.rect.y = bossArenaSpawn.y;
                cameraMode = 1; //static camera
                worldMode = 1; //boss arena background

            }
            // Jump with W or SPACE
            if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) && light == GREEN_LIGHT) {
                if (!player.isJumping && onPlatform) {          //Normal Jump
                    player.velocityY = JUMP_FORCE;
                    player.isJumping = true;
                } else if (player.doubleJumpCount > 0 && player.isJumping) {    //double jump
                    player.velocityY = JUMP_FORCE;
                    (player.doubleJumpCount)--;
                } else if (player.isWallSliding) {         // Wall jump
                    player.velocityY = JUMP_FORCE;
                    player.isJumping = true;
                    player.isWallSliding = false;

                    // Add horizontal push-off from wall
                    if (onRightWall) player.rect.x += 20.0f;  // push right
                    if (onLeftWall) player.rect.x -= 20.0f; // push left
                }
            }

            if (!player.isWallSliding) {    //fast gravity downwards if not sliding (accelerated every frame)
                player.velocityY += GRAVITY * player.gravitySign ;
                player.rect.y += player.velocityY;
            }
            else {
                player.velocityY = 1.0f * player.gravitySign;  // slow slide down    (constant velocity, change if necessary)
                player.rect.y += player.velocityY;
            }

            
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !player.isAttacking) {
                player_attack_animation1(&player_anim);
                player.isAttacking = true;
                player.isDealingDamage = true;
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !player.isAttacking) {
                player_attack_animation2(&player_anim);
                player.isAttacking = true;
                player.isDealingDamage = true;
            }

            //===============================================================================================//

            // idle animation if standing on platform and not moving/jumping
            if (!player.isAttacking) {
                if (onPlatform && !moving && !player.isJumping) {
                    if (player_anim.row != 0) { 
                        idle_animation(&player_anim);
                    }
                } else {
                    select_player_animation(moving, player.isJumping, &player_anim);
                }
            }
            animation_update(&player_anim);

            if (player.isAttacking && player_anim.current_frame >= player_anim.last_frame) {
                player.isAttacking = false;
                player.isDealingDamage = false;
                idle_animation(&player_anim);
            }

            // Check fall out of screen
            if (player.rect.y > screenHeight + 30) {
                player.isAlive = false;
                death_animation(&player_anim);
            }

            // RED LIGHT: detect any move
            if(worldMode==0){
            if (light == RED_LIGHT && (
                    IsKeyDown(KEY_A) || IsKeyDown(KEY_D) ||
                    IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) ||
                    IsKeyDown(KEY_W) || IsKeyDown(KEY_SPACE) ||
                    IsKeyDown(KEY_LEFT_SHIFT))){
                if (player.isAlive) {
                    player.isAlive = false;
                    death_animation(&player_anim);
                    
                }
            }
        }
            if (CheckCollisionRecs(player.rect, damageBlock)) {
                player.health--;
                hit_animation(&player_anim);

                if (player.health == 0) {
                    
                    player.isAlive = false;
                    hit_animation(&player_anim);
                    death_animation(&player_anim);
                    
                } 
                else {
                    player.rect.x -= 20; // knockback
                }
            }


            //======================================Teleport===============================================//

            if (CheckCollisionRecs(player.rect, teleportZoneA) && IsKeyPressed(KEY_UP)) {
                player.rect.x = teleportZoneB.x;
                player.rect.y = teleportZoneB.y - player.rect.height;
            }
            else if (CheckCollisionRecs(player.rect, teleportZoneB) && IsKeyPressed(KEY_UP)) {
                player.rect.x = teleportZoneA.x;
            player.rect.y = teleportZoneA.y - player.rect.height;
            }

            //======================================Mob Section=======================================//

            //mob hitbox centered on collider
            if(mob.mobHealth>0){
            mob.hitbox.x = mob.collider.x + (mob.collider.width - mob.hitbox.width) / 2; 
            mob.hitbox.y = mob.collider.y + mob.collider.height - mob.hitbox.height; 
            if (CheckCollisionRecs(player.rect, mob.collider)) {
                if (!mob.isActive) {
                    mob.isActive = true; 
                    mob.timer = EYEBALL_MOB_TIMER; //bug fix: activate mob on first collision 
                }
                mob.timer -= GetFrameTime();

                if(mob.timer <= 0){
                    
                    mob_attack_animation1(&mob_anim);
                    

                    player.health--; // Player takes damage
                    hit_animation(&player_anim);
                    
                    if (player.health == 0) {

                        
                        hit_animation(&player_anim);
                        player.rect.x -= 10; // knockback
                        death_animation(&player_anim);
                        player.isAlive = false;


                    } else {
                        player.rect.x -= 10; // knockback
                    }
                    mob.timer = EYEBALL_MOB_TIMER; 
                    
                }
            }
        }
            else {
                mob.isActive = false;
                if( mob_anim.row != 0){
                    mob_idle_animation(&mob_anim);
                }
                
            }
        }

        //check for player attack
        if (player.isDealingDamage && mob.isAlive && brain.isAlive) {
        //create an attack box larger than player hitbox
            Rectangle attackBox = player.rect;
            attackBox.width += 20.00f;


        // Check collision with mob
            if (CheckCollisionRecs(attackBox, mob.hitbox)) {
                mob.mobHealth--; // Damage mob
                player.isDealingDamage = false; //prevents multiple damage frames
            }

        }



        //================================== Eye Ball====================//

        // // Spawn every 5 seconds
        // dotTimer += GetFrameTime();
        // if (dotTimer >= 10.0f && !dotActive) {
        //     dotActive = true;
        //     dotTimer = 0;

        //     // Fixed spawn (always from left offscreen)
        //     dotPos = (Vector2){player.rect.x-400,player.rect.y-400};

            
        // }
        for (int i = 0; i < MAX_DOTS; i++) {
    dots[i].timer += GetFrameTime();

    if (dots[i].timer >= 6.0f && !dots[i].active) {
        float squareHalfSize = 600; // distance from player to edge

// Randomly decide horizontal or vertical edge
bool horizontal = GetRandomValue(0, 1); // 0=false (vertical), 1=true (horizontal)
float x, y;

if (horizontal) {
    y = player.rect.y + (GetRandomValue(0, 1) ? squareHalfSize : -squareHalfSize); // top or bottom
    x = player.rect.x + GetRandomValue(-squareHalfSize, squareHalfSize);           // random along width
} else {
    x = player.rect.x + (GetRandomValue(0, 1) ? squareHalfSize : -squareHalfSize); // left or right
    y = player.rect.y + GetRandomValue(-squareHalfSize, squareHalfSize);           // random along height
}

// Assign to dot
dots[i].pos = (Vector2){x, y};
dots[i].active = true;
dots[i].timer = 0;
    }
}

Vector2 mouseScreen = GetMousePosition();
float Scale = (float)GetScreenHeight() / virtualHeight;
int ScaledWidth = (int)(virtualWidth * Scale);
int OffsetX = (GetScreenWidth() - ScaledWidth) / 2;

Vector2 mouseVirtual = {
    (mouseScreen.x - OffsetX) / Scale,
    mouseScreen.y / Scale
};

// Convert to world coordinates with camera
Vector2 mouseWorld = GetScreenToWorld2D(mouseVirtual, camera);


        for (int i = 0; i < MAX_DOTS; i++) {
    if (dots[i].active) {
        
        Vector2 playerCenter = {
            player.rect.x + player.rect.width / 2,
            player.rect.y + player.rect.height / 2
        };

        if (dots[i].pos.x < playerCenter.x) dots[i].pos.x += 3.5;
        if (dots[i].pos.x > playerCenter.x) dots[i].pos.x -= 3.5;
        if (dots[i].pos.y < playerCenter.y) dots[i].pos.y += 3.5;
        if (dots[i].pos.y > playerCenter.y) dots[i].pos.y -= 3.5;

      
        if (CheckCollisionCircles(dots[i].pos, DOT_RADIUS, playerCenter, PLAYER_DRAW_SIZE / 2)) {
            player.health -= 1;      
            dots[i].active = false;  // remove the dot

            if (player.health <= 0) {  
                player.health = 0;      
                player.isAlive = false; 
                death_animation(&player_anim);
            }
        }

        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointCircle(mouseWorld, dots[i].pos, 20)) {
        dots[i].active = false;  // destroy dot
        }   
    }
}
        
        
        //==================================== Animation Updates =======================================//
        
        animation_update(&mob_anim);
        animation_update(&player_anim);
        animation_update(&boss_anim);




        //==================================== LASER LOGIC =======================================//


       // Activate laser when pressing E
        if (IsKeyPressed(KEY_E) && !laserActive && player.laserAcquired)
        {
            laserActive = true;
            laserTimer = 0.0f;
        }

        // Update laser
        if (laserActive)
        {
            laserTimer += GetFrameTime();

            // Follow player position
            if (player.facingDirection == 1) // right
            {
                laserRect.x = player.rect.x + player.rect.width;
                laserRect.width = laserLength;
            }
            else // left
            {
                laserRect.x = player.rect.x - laserLength;
                laserRect.width = laserLength;
            }
            laserRect.y = player.rect.y+15;
            if(player_anim.row == 4){ //if jumping, adjust laser height
                laserRect.y = player.rect.y+10;
            }
            laserRect.height = player.rect.height;
            laserRect.height = player.rect.height/20;

            if (laserTimer >= laserDuration)
                laserActive = false; // turn off after duration
        }


        //====================================== RESET =======================================//
        
        if (IsKeyPressed(KEY_Y)) {
            // Reset world mode
             //overworld
            
            
            worldMode = 0;
            player.rect.x = spawnPoint.x;
            player.rect.y = spawnPoint.y;
            cameraMode = 0; //dynamic camera
            
            
            player.gravitySign = 1;
            player.velocityY = 0;
            player.facingDirection = 1;
            player.isAlive = true;
            player.health = 100;
            player.phaseActive = false;

            player.isJumping = false;

            player.doubleJumpCount = 0;
            player.dashCount = 0;
            player.isDashing = false;

            //reset mob
            
            mob.mobHealth = 3;
            mob.isAlive = true;
            mob.isActive = false;
            mob.timer = EYEBALL_MOB_TIMER;

            // Reset animation to idle
            idle_animation(&player_anim);

            // Reset camera target
            

            //reset double jumps
            for (int i = 0; i < DOUBLE_JUMPS; i++) {
                Djumps[i].isCollected = false;
            }
            for (int i = 0; i < DASHES; i++) {
                Dashes[i].isCollected = false;
            }
            for( int i=0 ; i<LEVITATION; i++){
                Levitations[i].isCollected = false;
            }
            for( int i=0 ; i<NOCLIP; i++){
                Noclips[i].isCollected = false;
            }
        }


        if (IsKeyPressed(KEY_R)) {
            // Reset world mode
             //overworld
            
            
            // Reset player
            brain.brainHealth = 100;
        brain.isAlive = true;
            if(worldMode==0){
            player.rect.x = spawnPoint.x;
            player.rect.y = spawnPoint.y;
            cameraMode = 0; //dynamic camera
            
            }
            if(worldMode==1){
                player.rect.x = bossArenaSpawn.x;
                player.rect.y = bossArenaSpawn.y;
                cameraMode = 1; //static camera
            }
            player.gravitySign = 1;
            player.velocityY = 0;
            player.facingDirection = 1;
            player.isAlive = true;
            player.health = 100;

            player.isJumping = false;

            player.doubleJumpCount = 0;
            player.dashCount = 0;
            player.isDashing = false;

            //reset mob
            
            mob.mobHealth = 3;
            mob.isAlive = true;
            mob.isActive = false;
            mob.timer = EYEBALL_MOB_TIMER;

            // Reset animation to idle
            idle_animation(&player_anim);

            // Reset camera target
            

            //reset double jumps
            for (int i = 0; i < DOUBLE_JUMPS; i++) {
                Djumps[i].isCollected = false;
            }
            for (int i = 0; i < DASHES; i++) {
                Dashes[i].isCollected = false;
            }
            for( int i=0 ; i<LEVITATION; i++){
                Levitations[i].isCollected = false;
            }
            for( int i=0 ; i<NOCLIP; i++){
                Noclips[i].isCollected = false;
            }
        }
        for (int i = 0; i < CheckPointcount; i++) {
            if (CheckCollisionRecs(player.rect, Checkpoint[i])) {
                spawnPoint.x = Checkpoint[i].x;
                spawnPoint.y = Checkpoint[i].y;
            }
        }
        
        
        
        
        
        

  

        // Update camera to follow player
        if(cameraMode == 0) { //dynamic camera
            camera.target = (Vector2){player.rect.x + player.rect.width / 2, player.rect.y + player.rect.height / 2};
        }
        else if (cameraMode == 1) { //static camera for boss arena
            camera.target = (Vector2){bossArenaSpawn.x, bossArenaSpawn.y};
        }











        
        //====================================== BEGIN DRAWING =======================================//


        
       
        
        BeginTextureMode(target);
        ClearBackground(RAYWHITE);

        // Draw Worlds
        if (gamestarted)
        {
            if (player.isAlive)
            {
                // Overworld background
                if (worldMode == 0)
                {
                    DrawTexturePro(
                        boss_background,
                        (Rectangle){0, 0, boss_background.width, boss_background.height},
                        (Rectangle){0, 0, (float)virtualWidth, (float)virtualHeight},
                        (Vector2){0, 0},
                        0.0f,
                        WHITE
                    );

                    // Boss animation overlay
                    Texture2D current_boss_tex = (light == GREEN_LIGHT) ? boss_sleep_texture : boss_awake_texture;
                    Rectangle boss_frame = animation_frame(&boss_anim, boss_max_frames, boss_num_rows, current_boss_tex);
                    DrawTexturePro(
                        current_boss_tex,
                        boss_frame,
                        (Rectangle){0, 0, (float)virtualWidth, (float)virtualHeight},
                        (Vector2){0, 0},
                        0.0f,
                        WHITE
                    );
                }

                // Boss arena background
                if (worldMode == 1)
                {
                    DrawTexturePro(
                        boss_arena_background,
                        (Rectangle){0, 0, boss_arena_background.width, boss_arena_background.height},
                        (Rectangle){0, 0, (float)virtualWidth, (float)virtualHeight},
                        (Vector2){0, 0},
                        0.0f,
                        WHITE
                    );
                }

                // Begin camera 2D mode
                BeginMode2D(camera);

                // Draw tilemaps
                DrawTilemap(tileset, 5, 5, platform1, 100, 345, TILE_SIZE);
                DrawTilemap(tileset, 6, 4, platform2, 200, 280, TILE_SIZE); //{200, 280, 128, 192},
                DrawTilemap(tileset, 6, 6, platform3, 400, 100, TILE_SIZE);
                DrawTilemap(tileset, 4, 8, platform4, 700, 150, TILE_SIZE);
                DrawTilemap(tileset, 4, 8, platform5, 900, 250, TILE_SIZE);
                DrawTilemap(tileset, 5, 5, platform6, 1200, 260, TILE_SIZE);
                DrawTilemap(tileset, 5, 6, platform7, 1400, 100, TILE_SIZE);
                DrawTilemap(tileset, 5, 5, platform8, 1700, 200, TILE_SIZE);
                DrawTilemap(tileset, 5, 5, platform9, 1840, 300, TILE_SIZE);
                DrawTilemap(tileset, 5, 6, platform10, 2100, 250, TILE_SIZE);
                DrawTilemap(tileset, 4, 5, platform11, 2300, 200, TILE_SIZE);
                DrawTilemap(tileset, 4, 8, platform12, 2500, 50, TILE_SIZE);
                DrawTilemap(tileset, 5, 6, platform13, 2800, 150, TILE_SIZE);
                DrawTilemap(tileset, 5, 5, platform14, 3150, 180, TILE_SIZE);
                DrawTilemap(tileset, 4, 5, platform15, 3350, 100, TILE_SIZE);
                DrawTilemap(tileset, 3, 4, platform16, 3500, 140, TILE_SIZE);
                DrawTilemap(tileset, 3, 6, platform17, 3700, 220, TILE_SIZE);
                DrawTilemap(tileset, 4, 7, platform18, 3800, 100, TILE_SIZE);
                DrawTilemap(tileset, 5, 5, platform19, 4100, 50, TILE_SIZE);
                DrawTilemap(tileset, 4, 6, platform20, 4400, 90, TILE_SIZE);
                DrawTilemap(tileset, 8, 18, platform21, 4700, 60, TILE_SIZE);// teleporter platform
                DrawTilemap(tileset, 7, 10, platform22, 5400, 310, TILE_SIZE);
                DrawTilemap(tileset, 5, 8, platform23, 5800, 130, TILE_SIZE); //wizard platform
                DrawTilemap(tileset, 2, 5, platform24, 4800, 500, TILE_SIZE);
                DrawTilemap(tileset, 4, 6, platform25, 6100, 400, TILE_SIZE);
                DrawTilemap(tileset, 3, 4, platform26, 6400, 300, TILE_SIZE);
                DrawTilemap(tileset, 3, 5, platform27, 6700, 400, TILE_SIZE);
                DrawTilemap(tileset, 4, 8, platform28, 7000, 150, TILE_SIZE);
                DrawTilemap(tileset, 3, 5, platform29, 7170, 190, TILE_SIZE);
                DrawTilemap(tileset, 4, 8, platform30, 7500, 350, TILE_SIZE);
                DrawTilemap(tileset, 4, 8, platform31, 7670, 280, TILE_SIZE);
                DrawTilemap(tileset, 4, 6, platform32, 8100, 200, TILE_SIZE);
                DrawTilemap(tileset, 4, 5, platform33, 8400, 100, TILE_SIZE);
                DrawTilemap(tileset, 4, 8, platform34, 8700, 250, TILE_SIZE);
                DrawTilemap(tileset, 3, 5, platform35, 8900, 300, TILE_SIZE);
                DrawTilemap(tileset, 4, 6, platform36, 9300, 200, TILE_SIZE);
                DrawTilemap(tileset, 4, 7, platform37, 9700, 100, TILE_SIZE);
                DrawTilemap(tileset, 4, 8, platform38, 10100, 250, TILE_SIZE);
                DrawTilemap(tileset, 4, 6, platform39, 10300, 160, TILE_SIZE);
            



            
                // BOSS ARENA WALLS
            
                DrawTilemap(boss_arena_tileset, 32, 32, platform40, 20000, -20000, TILE_SIZE);
                DrawTilemap(boss_arena_tileset, 32, 32, platform41, 20000-1024, -20000-1024+200, TILE_SIZE);
                
                DrawTilemap(boss_arena_tileset, 32, 32, platform42, 20000+1024, -20000-1024+200, TILE_SIZE);
                
                DrawTilemap(boss_arena_tileset, 3, 3, platform43, 20000+928, -20000-200, TILE_SIZE);
                DrawTilemap(boss_arena_tileset, 3, 4, platform44, 20000+928-300, -20000-200-150, TILE_SIZE);
                //{20000+928-300, -20000-200-150, 96, 128}

                DrawTilemap(boss_arena_tileset, 3, 3, platform43, 20000, -20000-200, TILE_SIZE);
                DrawTilemap(boss_arena_tileset, 3, 4, platform44, 20000+250, -20000-200-150, TILE_SIZE);


                //Draw Teleporter
                DrawTexturePro(
                    teleporter_texture,
                    (Rectangle){0, 0, teleporter_texture.width, teleporter_texture.height},
                    (Rectangle){4700, 60-60, 64, 64},
                    (Vector2){0, 0}, 0.0f, WHITE
                );


                //BOSS ARENA TELEPORTER
                DrawTexturePro(
                    teleporter_texture,
                    (Rectangle){0, 0, teleporter_texture.width, teleporter_texture.height},
                    (Rectangle){4700, 60-60, 64, 64}, //change this once sadnan gives coords
                    (Vector2){0, 0}, 0.0f, WHITE
                );

                //Draw NPC
                DrawTexturePro(
                    wizard_texture,
                    (Rectangle){0, 0, wizard_texture.width, wizard_texture.height},
                    (Rectangle){5800+100, 150-80, 64, 64},
                    (Vector2){0, 0}, 0.0f, WHITE
                );
                // Full Screen EYEBALL fix
                // Vector2 mouseScreen = GetMousePosition();
                // float scale = (float)GetScreenHeight() / virtualHeight;
                // int scaledWidth = (int)(virtualWidth * scale);
                // int offsetX = (GetScreenWidth() - scaledWidth) / 2;

                // Vector2 mouseVirtual = {
                //     (mouseScreen.x - offsetX) / scale,
                //     mouseScreen.y / scale
                // };

                // // Convert to world coordinates with camera
                // Vector2 mouseWorld = GetScreenToWorld2D(mouseVirtual, camera);



                // // Check collision
                // if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                //     CheckCollisionPointCircle(mouseWorld, dotPos, 20)) {
                //     dotActive = false;
                // }

                //Draw Eyeball Projectile
                // if(dotActive)
                // {
                //     DrawTexturePro(
                //         eyeball,
                //         (Rectangle){0, 0, eyeball.width, eyeball.height},
                //         (Rectangle){dotPos.x - 10, dotPos.y - 10, 20, 20},
                //         (Vector2){0, 0}, 0.0f, WHITE
                //     );
                // }    

                for (int i = 0; i < MAX_DOTS; i++) {
    if (dots[i].active) {
        DrawTexturePro(
            eyeball,
            (Rectangle){0, 0, eyeball.width, eyeball.height},
            (Rectangle){dots[i].pos.x - DOT_RADIUS, dots[i].pos.y - DOT_RADIUS, DOT_RADIUS * 2, DOT_RADIUS * 2},
            (Vector2){0, 0},
            0.0f,
            WHITE
        );
    }
}

        

      





        

                // Draw damage block
                DrawRectangleRec(damageBlock, MAROON);

                // Draw collectibles
                for (int i = 0; i < DOUBLE_JUMPS; i++)
                {
                    if (!Djumps[i].isCollected)
                    {
                        DrawTexturePro(double_jump_texture,
                                    (Rectangle){0, 0, double_jump_texture.width, double_jump_texture.height},
                                    Djumps[i].rect,
                                    (Vector2){0, 0}, 0.0f, WHITE);
                    }
                }

                for (int i = 0; i < DASHES; i++)
                {
                    if (!Dashes[i].isCollected)
                    {
                        DrawTexturePro(dash_texture,
                                    (Rectangle){0, 0, dash_texture.width, dash_texture.height},
                                    Dashes[i].rect,
                                    (Vector2){0, 0}, 0.0f, WHITE);
                    }
                }

                for (int i = 0; i < LEVITATION; i++)
                {
                    if (!Levitations[i].isCollected)
                    {
                        DrawTexturePro(levitation_texture,
                                    (Rectangle){0, 0, levitation_texture.width, levitation_texture.height},
                                    Levitations[i].rect,
                                    (Vector2){0, 0}, 0.0f, WHITE);
                    }
                }

                for(int i = 0; i< NOCLIP; i++){
                    if(!Noclips[i].isCollected){
                         DrawRectangleRec(Noclips[i].rect, RED);
                    }
                }

                // Draw player
                Rectangle frame = animation_frame(&player_anim, max_frames, num_rows, player_texture);
                frame.width *= direction;
                DrawTexturePro(player_texture,
                            frame,
                            (Rectangle){player.rect.x + player.rect.width / 2 - PLAYER_DRAW_SIZE / 2,
                                        player.rect.y + player.rect.height / 2 - PLAYER_DRAW_SIZE / 2,
                                        PLAYER_DRAW_SIZE * direction,
                                        PLAYER_DRAW_SIZE},
                            (Vector2){0, 0}, 0.0f, WHITE);

                // Draw mob
                if (mob.mobHealth > 0)
                {
                    Rectangle mob_frame = animation_frame(&mob_anim, mob_max_frames, mob_num_rows, mob_texture);
                    DrawTexturePro(mob_texture,
                                mob_frame,
                                (Rectangle){mob.hitbox.x + mob.hitbox.width / 2 - MOB_DRAW_SIZE / 2,
                                            mob.hitbox.y + mob.hitbox.height / 2 - MOB_DRAW_SIZE / 2,
                                            MOB_DRAW_SIZE * direction,
                                            MOB_DRAW_SIZE - 50},
                                (Vector2){0, 0}, 0.0f, WHITE);
                }

                // Brain drawing
                if (brain.brainHealth > 0 && worldMode == 1)
                {
                    Vector2 brainCenter = {brain.position.x + brain.position.width / 2,
                                        brain.position.y + brain.position.height / 2};

                    DrawTexturePro(brain_texture,
                                (Rectangle){0, 0, brain_texture.width, brain_texture.height},
                                (Rectangle){brainCenter.x - brain_texture.width / 2,
                                            brainCenter.y - brain_texture.height / 2,
                                            brain_texture.width,
                                            brain_texture.height},
                                (Vector2){0, 0}, 0.0f, WHITE);
                }

                // Draw laser
                if (laserActive)
                    DrawRectangleRec(laserRect, WHITE);
                if (laserActive && brain.isAlive)
                {
                    Vector2 brainCenter = {brain.position.x + brain.position.width / 2,
                                        brain.position.y + brain.position.height / 2};
                    float brainRadius = brain.position.width / 2;
                    if (CheckCollisionCircleRec(brainCenter, brainRadius, laserRect))
                        if (brain.brainHealth > 0) brain.brainHealth--;
                }

                EndMode2D(); // End camera mode
            }
        } // player.isAlive & gamestarted

        EndTextureMode();

        // Draw to screen
        BeginDrawing();
        ClearBackground(BLACK);

        float scale = (float)GetScreenHeight() / virtualHeight;
        int scaledWidth = (int)(virtualWidth * scale);
        int scaledHeight = (int)(virtualHeight * scale);
        int offsetX = (GetScreenWidth() - scaledWidth) / 2;
        int offsetY = (GetScreenHeight() - scaledHeight) / 2;

        DrawTexturePro(target.texture,
                    (Rectangle){0, 0, (float)target.texture.width, -(float)target.texture.height},
                    (Rectangle){offsetX, offsetY, scaledWidth, scaledHeight},
                    (Vector2){0, 0}, 0.0f, WHITE);

        // HUD
        DrawText(TextFormat("Health: %d", player.health), 20, 20, 30, WHITE);

        if (worldMode == 1 && brain.brainHealth > 0)
        {
            int maxHealth = 100;
            float barWidth = 400;
            float barHeight = 25;
            float barX = GetScreenWidth() / 2 - barWidth / 2;
            float barY = 20;
            float currentWidth = (brain.brainHealth / (float)maxHealth) * barWidth;
            DrawRectangle(barX, barY, barWidth, barHeight, DARKGRAY);
            DrawRectangle(barX, barY, currentWidth, barHeight, RED);
            DrawRectangleLines(barX, barY, barWidth, barHeight, BLACK);
        }
        if (showDialogue)
        {
                Rectangle dialogueBox = { GetScreenWidth()/2 - 300, GetScreenHeight()/2 + 100, 800, 200 }; // bottom of screen
                DrawRectangleRec(dialogueBox, BLACK);
                DrawRectangleLines(dialogueBox.x, dialogueBox.y, dialogueBox.width, dialogueBox.height, WHITE);
                if(player.laserAcquired == true)
                {
                    DrawText(dialogueText1, dialogueBox.x + 10, dialogueBox.y + 10, 20, WHITE);
                }
                else if(player.laserAcquired == false)
                {
                    DrawText(dialogueText2, dialogueBox.x + 10, dialogueBox.y + 10, 20, WHITE);
                }
                
        }

        // Game over
        if (!player.isAlive)
        {
            DrawText("GAME OVER",
                    GetScreenWidth() / 2 - MeasureText("GAME OVER", 40) / 2,
                    GetScreenHeight() / 2 - 20, 40, RED);
            DrawText("Press R to Restart",
                    GetScreenWidth() / 2 - MeasureText("Press R to Restart", 20) / 2,
                    GetScreenHeight() / 2 + 20, 20, RED);
            
        }

        // Menu buttons if game not started
        if (!gamestarted)
        {
            playButton.x = GetScreenWidth()/2 - playButton.width/2;
            quitbutton.x = GetScreenWidth()/2 - quitbutton.width/2;
            playButton2.x = GetScreenWidth()/2 - playButton2.width/2;
            DrawRectangleRec(playButton, GRAY);
            DrawRectangleRec(playButton2, GRAY);
            DrawRectangleRec(quitbutton, GRAY);
            DrawText("REIGN OF CTHULHU",
                     GetScreenWidth()/2 - MeasureText("REIGN OF CTHULHU", 40)/2,
                     200, 40, RED);
            DrawText("LEVEL 1", playButton.x + 10, playButton.y + 10, 20, BLACK);
            DrawText("LEVEL 2", playButton2.x + 10, playButton2.y + 10, 20, BLACK);
            DrawText("QUIT", quitbutton.x + 20, quitbutton.y + 10, 20, BLACK);
        }

        EndDrawing();

    }

    UnloadTexture(player_texture);
    UnloadSound(awake_fx);
    CloseAudioDevice();
    CloseWindow();
    return 0;
    }
