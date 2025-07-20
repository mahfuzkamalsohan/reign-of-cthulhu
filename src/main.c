#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define PLAYER_DRAW_SIZE 100
#define MOB_DRAW_SIZE 350 

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 720
#define GRAVITY 0.5f
#define JUMP_FORCE -10.0f
#define PLAYER_SPEED 5.0f
#define MAX_PLATFORMS 6
#define DOUBLE_JUMPS 2
#define DASHES 2
#define TOTAL_TIME 120.0f // seconds (2min game timer)
#define DASH_DISTANCE 300.0f
#define DASH_STEP 60.0f
#define EYEBALL_MOB_TIMER 1.0f // seconds


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


    bool isAlive;       //alive status

    int health;         //health of player

    bool isJumping;     //if player is on air 
    bool isDashing;     //if player is dashing
    bool isWallSliding;      

    int doubleJumpCount;    //consumable double jumps

    int dashCount;      //consumable dashes
    float dashTargetX;  //
    int dashFrames;
    bool isAttacking; 
} Player;

typedef struct Mob {
    Rectangle collider;
    Rectangle hitbox; //mob hitbox
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
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Merged Platformer + Animation");
    const int TILE_SIZE = 32;  //each tile is 32x32 px

    Texture2D tileset = LoadTexture("assets/map/tileset.png");
    Texture2D player_texture = LoadTexture("assets/hero/hero.png");
    Texture2D mob_texture = LoadTexture("assets/enemies/eyehead.png");

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

    const int mob_max_frames = 10;
    const int mob_num_rows = 6;

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
        .rect = {100,300, PLAYER_DRAW_SIZE/4, PLAYER_DRAW_SIZE/2},
        .velocityY = 0,
        .facingDirection = 1,

        .isAlive = true,

        .health = 3,

        .isJumping = false,

        .doubleJumpCount = 0,

        .dashCount = 0,
        .isDashing = false,
        .isAttacking = false,
    };


    Mob mob = {
        .collider = {400, 50, 200, 50},
        .hitbox = {400, 50, 20, 50},
        .isAlive = true,
        .isActive = false,
        .timer = EYEBALL_MOB_TIMER
    };






   //======================================Level Layout PLATFORMS=========================================//

    //add platforms with physics
    Rectangle platforms[MAX_PLATFORMS] = {
        {100, 345, 160, 160},
        {200, 280, 128, 192},
        {400, 100, 192, 192}
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
		{TOP_LEFT_ANGLE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {CENTER_TILE, RIGHT_BOTTOM_ANGLE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
	};

    int platform3[6][6] = {
        {LEFT_TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, TOP_TILE, RIGHT_TOP_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, CENTER_TILE, RIGHT_TILE},
        {LEFT_BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, BOTTOM_TILE, RIGHT_BOTTOM_TILE}
    };

    //temporary damage block
    Rectangle damageBlock = { 300, -2000, 50, 50 }; 






    //add power ups
    PowUpDjump Djumps[DOUBLE_JUMPS] = {
    //    { {100, 0, 20, 20}, false },
       { {100,40,20,20}, false }
    };

    PowUpDash Dashes[DASHES] = {
        { {300,-40,20,20}, false }
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
                        if (overlapLeft < overlapRight) {       //if left overlap less, move to left of platform 
                            player.rect.x -= overlapLeft;
                        }
                        else {
                            player.rect.x += overlapRight;
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





            


            
            

            //======================================Movement Section/Controls=======================================//

            if ((onLeftWall || onRightWall) && !onPlatform) {        //wallslide if player touching right/left wall
            
                player.isWallSliding = true;
                player.isJumping = true;
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
                player.velocityY += GRAVITY;
                player.rect.y += player.velocityY;
            }
            else {
                player.velocityY = 1.0f;  // slow slide down    (constant velocity, change if necessary)
                player.rect.y += player.velocityY;
            }

            
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !player.isAttacking) {
                player_attack_animation1(&player_anim);
                player.isAttacking = true;
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !player.isAttacking) {
                player_attack_animation2(&player_anim);
                player.isAttacking = true;
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
                idle_animation(&player_anim);
            }

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




            //======================================Mob Section=======================================//

            //mob hitbox centered on collider
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
            else {
                mob.isActive = false;
                if( mob_anim.row != 0){
                    mob_idle_animation(&mob_anim);
                }
                
            }

        }
        
        
        //==================================== Animation Updates =======================================//
        
        animation_update(&mob_anim);
        animation_update(&player_anim);







        //====================================== RESET =======================================//
            
        if (IsKeyPressed(KEY_R)) {
            // Reset player
            player.rect.x = 100;
            player.rect.y = 300;
            player.velocityY = 0;
            player.facingDirection = 1;
            player.isAlive = true;
            player.health = 3;

            player.isJumping = false;

            player.doubleJumpCount = 0;
            player.dashCount = 0;
            player.isDashing = false;

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


  

        // Update camera to follow player
        camera.target = (Vector2){player.rect.x + player.rect.width / 2, player.rect.y + player.rect.height / 2};










        
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
    









        // Draw damage block
        DrawRectangleRec(damageBlock, MAROON);

        //DrawRectangleRec(mob.collider, DARKPURPLE);
        //DrawRectangleRec(mob.hitbox, PURPLE);

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


        Rectangle mob_frame = animation_frame(&mob_anim, mob_max_frames, mob_num_rows, mob_texture);


        DrawTexturePro(
            mob_texture,
            mob_frame,
            (Rectangle){
                mob.hitbox.x + mob.hitbox.width / 2 - MOB_DRAW_SIZE / 2,
                mob.hitbox.y + mob.hitbox.height / 2 - MOB_DRAW_SIZE / 2,
                MOB_DRAW_SIZE * direction,
                MOB_DRAW_SIZE - 50
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


        //Draw INSTRUCTIONS
        DrawText("LEFT CLICK: LIGHT ATTACK", 750, 10, 20, BLACK);
        DrawText("RIGHT CLICK: HEAVY ATTACK", 750, 40, 20, BLACK);

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
