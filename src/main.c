#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>

#define PLAYER_DRAW_SIZE 100

#define SCREEN_WIDTH 2160
#define SCREEN_HEIGHT 720
#define GRAVITY 0.5f
#define JUMP_FORCE -12.0f
#define PLAYER_SPEED 5.0f
#define MAX_PLATFORMS 100
#define DOUBLE_JUMPS 50
#define DASHES 50
#define TOTAL_TIME 120.0f // seconds (2min game timer)
#define DASH_DISTANCE 200.0f

Rectangle teleportZoneA = { 6100, 60, 576, 256};  
Rectangle teleportZoneB = { 6200, 500, 160, 64 }; 

Vector2 spawnPoint = {130, 345};

#define DamageBlocks 20

Vector2 dotPos = {-1000,-1000};
bool dotActive = false;
float dotTimer = 0;

#define CheckPointcount 10

Rectangle Checkpoint[CheckPointcount]= 
{
    {1750,100,32,32},
    {3600,150,32,32},
    {5300, 50,32,32},
    {7230, 130,32,32},
    {9830, 100,32,32},
    {11730, 160,32,32},
    {12950, 0,32,32}
};

typedef enum AnimationType { LOOP = 1, ONESHOT = 2 } AnimationType;
typedef enum Direction { LEFT = -1, RIGHT = 1 } Direction;
typedef enum LightState { RED_LIGHT = 0, GREEN_LIGHT = 1 } LightState;
typedef enum TILE_TYPE { TOP_TILE = 0, LEFT_TILE = 1, RIGHT_TILE = 2, BOTTOM_TILE = 3, CENTER_TILE = 4, LEFT_TOP_TILE = 5, RIGHT_TOP_TILE = 6, LEFT_BOTTOM_TILE = 7, RIGHT_BOTTOM_TILE = 8} TILE_TYPE;

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
    Rectangle rect;
    float velocityY;
    bool isJumping;
    bool isDashing;
    bool isAlive;
    int health;

    int doubleJumpCount;
    int dashCount;
} Player;

typedef struct Mob {
    Rectangle rect;
    bool isAlive;
    bool isCharged;
} Mob;

typedef struct PowUpDjump {
    Rectangle rect;
    bool isCollected;
} PowUpDjump;

typedef struct PowUpDash {
    Rectangle rect;
    bool isCollected;
} PowUpDash;



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

void death_animation(Animation* anim) {
    anim->row = 7;
    anim->first_frame = 0;
    anim->last_frame = 13;
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

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Merged Platformer + Animation");
    const int TILE_SIZE = 32;  //each tile is 32x32 px

    Texture2D tileset = LoadTexture("assets/map/tileset.png");
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

//======================================Player Initialization=================================//

    Player player = {
        //player collision box x, y, width, height
        .rect = {spawnPoint.x,spawnPoint.y, PLAYER_DRAW_SIZE/4, PLAYER_DRAW_SIZE/2},
        .velocityY = 0,
        .isJumping = false,
        .isDashing = false,
        .isAlive = true,
        .health = 3,

        .doubleJumpCount = 0,
        .dashCount = 0
    };

    Mob mob = {
        .rect = {500, 250, 50, 50},
        .isAlive = true,
        .isCharged = false
    };






   //======================================Level Layout PLATFORMS=========================================//

    //add platforms with physics
  Rectangle platforms[MAX_PLATFORMS] = {
        {100, 345, 160, 160},//1
        {200, 280, 128, 192},//2
        {400, 100, 192, 192},//3
        {700, 150, 256, 128},//4
        {900, 250, 256, 128},//5
        {1300, 260, 160, 160},//6
        {1700, 100, 192, 160},//7
        {2100, 200, 160, 160},//8
        {2250, 300, 160, 160},//9
        {2650, 250, 192, 128},//10
        {2800, 200, 160, 128},//11
        {3150, 50, 256, 128},//12
        {3550, 150, 192, 128},//13
        {4000, 180, 192, 160},//14
        {4350, 100, 160, 128},//15
        {4500, 140, 128, 96},//16
        {4800, -100, 192, 96},//17
        {4950, -200, 224, 128},//18
        {5275, 50, 160, 160},//19
        {5700, 90, 192, 128},//20
        {6100, 60, 576, 256},//21 
        {6800, 310, 320, 224},//22
        {7200, 130, 256, 160},//23
        {6200, 500, 160, 64},//24
        {7500, 400, 192, 128},//25
        {7800, 300, 160, 128},//26
        {8100, 400, 160, 96},//27
        {8400, 150, 256, 128},//28
        {8570, 190, 160, 96},//29
        {8900, 350, 256, 128},//30
        {9070, 280, 256, 128},//31
        {9500, 200, 192, 128},//32
        {9800, 100, 160, 128},//33
        {10100, 250, 256, 128},//34
        {10300, 300, 160, 96},//35
        {10700, 200, 192, 128},//36
        {11100, 100, 224, 128},//37
        {11500, 250, 256, 128},//38
        {11700, 160, 192, 128},//39
        {12000, 350, 96, 128},//40
        {12000, 600, 320, 224},//41
        {12550, 450, 96, 192},//42
        {12450, 200, 96,192},//43
        {12550, -50, 96, 192},//44
        {12900, 0, 192, 96},//45
        {13250, -100, 256, 128},//46
        {13800, 100, 256, 160},//47
        {14000, 210, 160, 128},//48
        {14400, 170, 288, 128}//49
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
    {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
};

int platform18[4][7] = {
    {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
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
int platform60[4][3] = {
    {LEFT_TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
};
int platform61[7][10] = {
    {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
};
int platform62[6][3] = {
    {LEFT_TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
};
int platform63[6][3] = {

    {LEFT_TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
};
int platform64[6][3] = {

    {LEFT_TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
};
int platform65[3][6] = { 
    {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE },
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
};
int platform66[4][8] = {
    {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
};
int platform67[5][8] = {
    {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, CENTER_TILE}
};
int platform68[4][5] = {
    {CENTER_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
};
int platform69[4][9] = {
    {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
    {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
};

    //temporary damage block
    Rectangle damageBlock[DamageBlocks] = { 
        {300, 200, 50, 50},
        {1550,200,50,50},
        {2500,250, 50, 50},
        {3850,100,50,50},
        {4700, 100,50,50},
        {5550,100,50,50},
        {8350,350,50,50},
        {9370,170,50,50},
        {10550,250,50,50},
        {11000,150,50,50},
        {12650,300,50,50},
        {13680,-50,50,50}
     }; 






    //add power ups
    PowUpDjump Djumps[DOUBLE_JUMPS] = {
       { {1350, 160,20,20}, false },
       { {4600, 40, 20,20}, false},
       { {6280, 450, 20,20}, false},
       { {7350, 70, 20, 20}, false}
    };

    PowUpDash Dashes[DASHES] = {
        { {450, 50, 20, 20}, false},
        { {5050, -300,20,20}, false},
        { {6250, 450, 20,20}, false},
        { {9000, 250, 20, 20}, false}
    };






//====================================Camera Setting========================================//


    // Setting up camera
    Camera2D camera = {0};
    camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    camera.target = (Vector2){player.rect.x + player.rect.width / 2, player.rect.y + player.rect.height / 2};
    camera.zoom = 1.0f;

    Direction direction = RIGHT;
    LightState light = GREEN_LIGHT;
    double state_start_time = GetTime();
    Vector2 last_pos = {player.rect.x, player.rect.y};
    
    // Add at the top of main(), after Animation player_anim = {...};
    int last_anim_row = -1;

//===================================main game loop=======================================//

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        float dt = GetFrameTime();
        double elapsed = GetTime() - state_start_time;
        if (player.isAlive) {

            // Light state logic
            if (light == GREEN_LIGHT && elapsed >= 10.0) {
                light = RED_LIGHT;
                state_start_time = GetTime();
            } else if (light == RED_LIGHT && elapsed >= 1.0) {
                light = GREEN_LIGHT;
                state_start_time = GetTime();   
            }

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

            //============================Collision part===========================================//

            // Detect collisions for jump
            bool onLeftWall = false;    //touching left side of wall
            bool onRightWall = false;   //touching right side of wall
            bool onPlatform = false;    //standing on platform

            for (int i = 0; i < MAX_PLATFORMS; i++) {
                Rectangle plat = platforms[i];  //check each platform one by one

                if (CheckCollisionRecs(player.rect, plat)) {
                    if (player.rect.x + player.rect.width > plat.x &&
                        player.rect.x < plat.x &&
                        player.rect.y + player.rect.height > plat.y &&
                        player.rect.y < plat.y + plat.height) {
                        onRightWall = true;         // Check collision with right side of platform
                    }

                    if (player.rect.x < plat.x + plat.width &&
                        player.rect.x + player.rect.width > plat.x + plat.width &&
                        player.rect.y + player.rect.height > plat.y &&
                        player.rect.y < plat.y + plat.height) {
                        onLeftWall = true;          // Check collision with left side of platform
                    }

                    // Calculate how much the player overlaps the platform
                    float overlapLeft = (player.rect.x + player.rect.width) - plat.x;
                    float overlapRight = (plat.x + plat.width) - player.rect.x;
                    float overlapTop = (player.rect.y + player.rect.height) - plat.y;
                    float overlapBottom = (plat.y + plat.height) - player.rect.y;

                    // Find smallest overlap to determine collision direction
                    float minOverlapX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
                    float minOverlapY = (overlapTop < overlapBottom) ? overlapTop : overlapBottom;

                    if (minOverlapX < minOverlapY) {            // Horizontal collision
                        if (overlapLeft < overlapRight) {       //collision from left side
                            player.rect.x -= overlapLeft;       //move player position to negate overlap
                        } else {                                //collision from right side
                            player.rect.x += overlapRight;
                        }
                    } else {                                    // Vertical collision
                        if (overlapTop < overlapBottom) {       // Collided with top of platform
                            player.rect.y -= overlapTop;
                            player.velocityY = 0;
                            player.isJumping = false;
                            onPlatform = true;
                        } else {                                // Collided with bottom of platform
                            player.rect.y += overlapBottom;
                            player.velocityY = 0;
                        }
                    }
                }
            }

            if (!onPlatform) player.isJumping = true;           // If not on a platform, player is jumping





            


            
            

            //======================================Movement Section=======================================//

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

            //Dash with LShift
            if (IsKeyPressed(KEY_LEFT_SHIFT) && player.dashCount > 0) {
                if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
                    player.rect.x -= DASH_DISTANCE;
                    player.dashCount--;
                } else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
                    player.rect.x += DASH_DISTANCE;
                    player.dashCount--;
                }
            }

            // Jump with W or SPACE
            if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) && light == GREEN_LIGHT) {
                if (!player.isJumping && onPlatform) {          //Normal Jump
                    player.velocityY = JUMP_FORCE;
                    player.isJumping = true;
                } else if (player.doubleJumpCount > 0 && player.isJumping) {
                    player.velocityY = JUMP_FORCE;
                    (player.doubleJumpCount)--;
                } else if (onLeftWall || onRightWall) {         // Wall jump
                    player.velocityY = JUMP_FORCE;
                    player.isJumping = true;

                    // Add horizontal push-off from wall
                    if (onLeftWall) player.rect.x += 25.0f;  // push right
                    if (onRightWall) player.rect.x -= 25.0f; // push left
                }
            }

            player.velocityY += GRAVITY;
            player.rect.y += player.velocityY;

            //===============================================================================================//

            // idle animation if standing on platform and not moving/jumping
            if (onPlatform && !moving && !player.isJumping) {
                if (player_anim.row != 0) { 
                    idle_animation(&player_anim);
                }
            } else {
                select_player_animation(moving, player.isJumping, &player_anim);
            }
            animation_update(&player_anim);

            // Check fall out of screen
            if (player.rect.y > SCREEN_HEIGHT + 30) {
                player.isAlive = false;
                death_animation(&player_anim);
            }

            // RED LIGHT: detect any move
            if (light == RED_LIGHT && (
                    IsKeyDown(KEY_A) || IsKeyDown(KEY_D) ||
                    IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) ||
                    IsKeyDown(KEY_W) || IsKeyDown(KEY_SPACE) ||
                    IsKeyDown(KEY_LEFT_SHIFT))) {
                if (player.isAlive) {
                    player.isAlive = false;
                    death_animation(&player_anim);
                    
                }
            }
            for(int i=0;i<DamageBlocks;i++){
                if (CheckCollisionRecs(player.rect, damageBlock[i])) {
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
        }



            if (CheckCollisionRecs(player.rect, mob.rect)) {
                if (!mob.isCharged) {
                    mob.isCharged = true; // Mob is now charged
                    player.health--; // Player takes damage
                    hit_animation(&player_anim);

                    if (player.health == 0) {
                        player.isAlive = false;
                        death_animation(&player_anim);
                    }
                    else {
                    player.rect.x -= 20; // knockback
                    }

                    mob.isCharged = false; // Reset mob state after charging

                }
                 
            }


        } 
        
        
        
        
        else {
            animation_update(&player_anim);



            //====================================== RESET =======================================//

            if (IsKeyPressed(KEY_R)) {
                // Reset player
                player.rect.x = spawnPoint.x;
                player.rect.y = spawnPoint.y;
                player.velocityY = 0;
                player.isJumping = false;
                player.isAlive = true;
                player.health = 3;

                // Reset animation to idle
                idle_animation(&player_anim);

                // Reset camera target
                camera.target = (Vector2){player.rect.x + player.rect.width / 2, player.rect.y + player.rect.height / 2};

                //reset double jumps
                for (int i = 0; i < DOUBLE_JUMPS; i++) {
                    Djumps[i].isCollected = false;
                }
                for (int i = 0; i < DASHES; i++) {
                    Dashes[i].isCollected = false;
                }
            }
        }

        // Update camera to follow player
        camera.target = (Vector2){player.rect.x + player.rect.width / 2, player.rect.y + player.rect.height / 2};




       //======================================Teleport===============================================//
       
        if (CheckCollisionRecs(player.rect, teleportZoneA) && IsKeyPressed(KEY_UP)) {
            player.rect.x = teleportZoneB.x;
            player.rect.y = teleportZoneB.y - player.rect.height;
        }
        else if (CheckCollisionRecs(player.rect, teleportZoneB) && IsKeyPressed(KEY_UP)) {
            player.rect.x = teleportZoneA.x;
         player.rect.y = teleportZoneA.y - player.rect.height;
        }



        //================================== Eye Ball====================//

        // Spawn every 5 seconds
        dotTimer += GetFrameTime();
        if (dotTimer >= 5.0f && !dotActive) {
            dotActive = true;
            dotTimer = 0;

            // Fixed spawn (always from left offscreen)
            dotPos = (Vector2){-1000,-1000};

            
}

        //===============================CheckPoint===========================//

   for (int i = 0; i < CheckPointcount; i++) {
    if (CheckCollisionRecs(player.rect, Checkpoint[i])) {
        spawnPoint.x = Checkpoint[i].x;
        spawnPoint.y = Checkpoint[i].y;
    }
}



        if (dotActive) {
            // get player center
        Vector2 playerCenter = {
         player.rect.x + player.rect.width/2,
         player.rect.y + player.rect.height/2
        };

        // Move toward player using < >
        if (dotPos.x < playerCenter.x) dotPos.x += 2;
        if (dotPos.x > playerCenter.x) dotPos.x -= 2;
        if (dotPos.y < playerCenter.y) dotPos.y += 2;
        if (dotPos.y > playerCenter.y) dotPos.y -= 2;

        // Collision with player
        if (CheckCollisionCircles(dotPos, 10, playerCenter, PLAYER_DRAW_SIZE/2)) {
            player.health -= 1;      // subtract health
                dotActive = false;       // remove the dot

    if (player.health <= 0) {  // check if player died
        player.health = 0;      // prevent negative health
        player.isAlive = false; // mark player dead
        death_animation(&player_anim);
    }
}



        // Convert mouse position to world coordinates
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);

        // Destroy on mouse click
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointCircle(mouseWorld, dotPos, 10)) {
            dotActive = false;
        }
    }


        
        //====================================== BEGIN DRAWING =======================================//


        BeginDrawing();
        ClearBackground(light == GREEN_LIGHT ? SKYBLUE : RED);

        char coordText[64];
        sprintf(coordText, "X: %.2f  Y: %.2f", player.rect.x, player.rect.y);
        DrawText(coordText, 10, 10, 20, BLACK);

        
        

        BeginMode2D(camera);
        

     

    //     Rectangle platforms[MAX_PLATFORMS] = {
    //     {100, 345, 100, 80},
    //     {200, 280, 120, 180},
    //     {500, 280, 1000, 80},
    //     {800, 200, 140, 800},
    //     {-50,-50,50,120},
    //     {200,0,200,180}
    // };

        // Draw platforms
        // for (int i = 0; i < MAX_PLATFORMS; i++) {
        //     DrawRectangleRec(platforms[i], GREEN);
        // }

        




        // Draw tilemap
        DrawTilemap(tileset, 5, 5, platform1, 100, 345, TILE_SIZE);
        DrawTilemap(tileset, 6, 4, platform2, 200, 280, TILE_SIZE);
        DrawTilemap(tileset, 6, 6, platform3, 400, 100, TILE_SIZE);
        DrawTilemap(tileset, 4, 8, platform4, 700, 150, TILE_SIZE);
        DrawTilemap(tileset, 4, 8, platform5, 900, 250, TILE_SIZE);
        DrawTilemap(tileset, 5, 5, platform6, 1300, 260, TILE_SIZE);
        DrawTilemap(tileset, 5, 6, platform7, 1700, 100, TILE_SIZE);
        DrawTilemap(tileset, 5, 5, platform8, 2100, 200, TILE_SIZE);
        DrawTilemap(tileset, 5, 5, platform9, 2250, 300, TILE_SIZE);
        DrawTilemap(tileset, 5, 6, platform10, 2650, 250, TILE_SIZE);
        DrawTilemap(tileset, 4, 5, platform11, 2800, 200, TILE_SIZE);
        DrawTilemap(tileset, 4, 8, platform12, 3150, 50, TILE_SIZE);
        DrawTilemap(tileset, 5, 6, platform13, 3550, 150, TILE_SIZE);
        DrawTilemap(tileset, 5, 5, platform14, 4000, 180, TILE_SIZE);
        DrawTilemap(tileset, 4, 5, platform15, 4350, 100, TILE_SIZE);
        DrawTilemap(tileset, 3, 4, platform16, 4500, 140, TILE_SIZE);
        DrawTilemap(tileset, 3, 6, platform17, 4800, -100, TILE_SIZE);
        DrawTilemap(tileset, 4, 7, platform18, 4950, -200, TILE_SIZE);
        DrawTilemap(tileset, 5, 5, platform19, 5275, 50, TILE_SIZE);
        DrawTilemap(tileset, 4, 6, platform20, 5700, 90, TILE_SIZE);
        DrawTilemap(tileset, 8, 18, platform21, 6100, 60, TILE_SIZE); 
        DrawTilemap(tileset, 7, 10, platform22, 6800, 310, TILE_SIZE);
        DrawTilemap(tileset, 5, 8, platform23, 7200, 130, TILE_SIZE);
        DrawTilemap(tileset, 2, 5, platform24, 6200, 500, TILE_SIZE);
        DrawTilemap(tileset, 4, 6, platform25, 7500, 400, TILE_SIZE);
        DrawTilemap(tileset, 3, 4, platform26, 7800, 300, TILE_SIZE);
        DrawTilemap(tileset, 3, 5, platform27, 8100, 400, TILE_SIZE);
        DrawTilemap(tileset, 4, 8, platform28, 8400, 150, TILE_SIZE);
        DrawTilemap(tileset, 3, 5, platform29, 8570, 190, TILE_SIZE);
        DrawTilemap(tileset, 4, 8, platform30, 8900, 350, TILE_SIZE);
        DrawTilemap(tileset, 4, 8, platform31, 9070, 280, TILE_SIZE);
        DrawTilemap(tileset, 4, 6, platform32, 9500, 200, TILE_SIZE);
        DrawTilemap(tileset, 4, 5, platform33, 9800, 100, TILE_SIZE);
        DrawTilemap(tileset, 4, 8, platform34, 10100, 250, TILE_SIZE);
        DrawTilemap(tileset, 3, 5, platform35, 10300, 300, TILE_SIZE);
        DrawTilemap(tileset, 4, 6, platform36, 10700, 200, TILE_SIZE);
        DrawTilemap(tileset, 4, 7, platform37, 11100, 100, TILE_SIZE);
        DrawTilemap(tileset, 4, 8, platform38, 11500, 250, TILE_SIZE);
        DrawTilemap(tileset, 4, 6, platform39, 11700, 160, TILE_SIZE);
        DrawTilemap(tileset, 4, 3, platform60, 12000, 350, TILE_SIZE);
        DrawTilemap(tileset, 7, 10, platform61, 12000, 600, TILE_SIZE);
        DrawTilemap(tileset, 6, 3, platform62, 12550, 450, TILE_SIZE);
        DrawTilemap(tileset, 6, 3, platform63, 12450, 200, TILE_SIZE);
        DrawTilemap(tileset, 6, 3, platform64, 12550, -50, TILE_SIZE);
        DrawTilemap(tileset, 3, 6, platform65, 12900, 0, TILE_SIZE);
        DrawTilemap(tileset, 4, 8, platform66, 13250, -100, TILE_SIZE);
        DrawTilemap(tileset, 5, 8, platform67, 13800, 100, TILE_SIZE);
        DrawTilemap(tileset, 4, 5, platform68, 14000, 210, TILE_SIZE);
        DrawTilemap(tileset, 4, 9, platform69, 14400, 170, TILE_SIZE);
    

        if (dotActive) {
    DrawCircleV(dotPos, 10, GREEN);
}



        // Draw damage block
       // Draw all damage blocks
        for (int i = 0; i < DamageBlocks; i++)
        {
         DrawRectangleRec(damageBlock[i], MAROON);
        }


        // Draw mob
        if (mob.isAlive) {
            DrawRectangleRec(mob.rect, BLUE);
        }
        else {
            DrawRectangleRec(mob.rect, DARKBLUE); // Draw dead mob
        }

        //Draw double jumps
        for (int i = 0; i < DOUBLE_JUMPS; i++) {
            if (Djumps[i].isCollected == false) {
                DrawRectangleRec(Djumps[i].rect, RED);
            }
        }

        //draw dashes
        for (int i = 0; i < DASHES; i++) {
            if (Dashes[i].isCollected == false) {
                DrawRectangleRec(Dashes[i].rect, GREEN);
            }
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

        //Draw player health
        DrawText(TextFormat("Health: %d", player.health), 500, 10, 20, WHITE);
        

        DrawText(light == GREEN_LIGHT ? "GREEN LIGHT: MOVE!" : "RED LIGHT: DON'T MOVE!", 20, 20, 20, WHITE);

        // Game over message
        if (!player.isAlive) {
            DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, SCREEN_HEIGHT / 2 - 20, 40, DARKGRAY);
            DrawText("Press R to Restart", SCREEN_WIDTH / 2 - MeasureText("Press R to Restart", 20) / 2, SCREEN_HEIGHT / 2 + 20, 20, DARKGRAY);
        }

        
        EndDrawing();
    }

    UnloadTexture(player_texture);
    CloseWindow();
    return 0;
} 