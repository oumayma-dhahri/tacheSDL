#ifndef HEADER_H
#define HEADER_H

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>

#define SCREEN_WIDTH 1700
#define SCREEN_HEIGHT 900
#define MAX_COINS 10
#define FRAME_WIDTH 150
#define FRAME_HEIGHT 150
#define TOTAL_FRAMES 9
#define MAX_ENEMIES 2

#define ENEMY2_IDLE         0
#define ENEMY2_WALK         1
#define ENEMY2_ATTACK       2
#define ENEMY2_HIT          3
#define ENEMY2_DEATH        4

enum HeroState {
    HERO_ATTACK,
    HERO_DEATH,
    HERO_FALL,
    HERO_HIT,
    HERO_IDLE,
    HERO_JUMP,
    HERO_RUN,
    HERO_WALK
};

typedef struct {
    SDL_Surface *spriteSheet;
    SDL_Rect *frames;
    int totalFrames;
} Animation;

typedef struct {
    float x, y;
    int active;
    int currentFrame;
    Uint32 frameTimer;
    int frameDelay;
    SDL_Surface *spriteSheet;
    SDL_Rect *frames;
    int totalFrames;
    float speedX;
    float amplitude;
    float frequency;
    float initialY;
    SDL_Rect rect;
} Coin;

typedef struct {
    float x, y;
    int direction;
    int state;
    int health;
    int maxHealth;
    float moveSpeed;
    Animation *animations;
    int numStates;
    SDL_Rect rect;
    int leftBoundary;
    int rightBoundary;
    int moving;
    int moveDirection;
    int currentFrame;
    Uint32 frameDelay;
    Uint32 frameTimer;
    int isFullyVisible;
    int animationPlaying;
    Uint32 hurtStartTime;
    int active;
    float meleeRange;
    int enemyType;
    float attackRange;
    float retreatDistance;
    Uint32 attackTimer;
    int attackPhase;
    float attackDistance;
    float initialY;
    float yAmplitude;
    float yFrequency;
    Uint32 yMoveStartTime;
    Uint32 yMoveDuration;
    int yMoveActive;
    int moveDirectionY;
    float yMoveSpeed;
    float yTarget;
    int oscillationCount;
    float oscillationPhase;
    int moveCount;
    float sinAmplitude;
    float sinFrequency;
} Enemy2;

typedef struct {
    float x, y;
    int direction;
    enum HeroState state;
    int health;
    int maxHealth;
    float moveSpeed;
    Animation animations[8];
    SDL_Surface *flippedSpriteSheet;
    SDL_Rect rect;
    int currentFrame;
    Uint32 frameTimer;
    Uint32 frameDelay;
    int isJumping;
    float jumpVelocity;
    float gravity;
    int isAttacking;
    Uint32 lastUpdateTime;
    int flip;
    int animationPlaying;
    int attackDamage;
    Uint32 lastHitTime;
} Hero;

typedef struct {
    int count;
    TTF_Font *font;
    SDL_Surface *textSurface;
    SDL_Rect position;
} Money;

typedef struct {
    SDL_Surface *background;
    Enemy2 enemies[MAX_ENEMIES];
} Level1;

typedef struct {
    SDL_Surface *background;
    Enemy2 enemies[2];
    int currentEnemy;
} Level2;

typedef struct {
    SDL_Surface *image;
    SDL_Rect position;
    SDL_Surface *healthBar;
    SDL_Rect healthBarPos;
} UI;

typedef struct {
    float x, y;
    int active;
    int currentFrame;
    Uint32 frameTimer;
    int frameDelay;
    SDL_Surface *spriteSheet;
    Animation animations[3];
    int currentAnimation;
} CollisionEffect;

int getHeroY();
int getEnemy1Y();
int getEnemy2Y();
int getEnemy3Y();
int getEnemy4Y();
int getCoinY();
SDL_Rect getHeroUIPos();
SDL_Rect getEnemyUIPos();
SDL_Rect getHeroHealthTextPos(SDL_Rect uiPos);
SDL_Rect getEnemyHealthTextPos(SDL_Rect uiPos);

SDL_Surface* CreateHealthBarSurface(TTF_Font *font, int health, int maxHealth);
SDL_Surface* CreateHeroHealthBarSurface(TTF_Font *font, Hero *hero);
SDL_Surface* ResizeSurface(SDL_Surface *src, float scale);

void InitCoin(Coin *coin, float startX, float startY);
void UpdateCoin(Coin *coin);
void RenderCoin(SDL_Surface *screen, Coin *coin);
void FreeCoin(Coin *coin);

void InitEnemy2(Enemy2 *e, const char *spriteSheetPath, int frameWidth, int frameHeight,
                int *framesPerState, int numStates, int maxHealth, int enemyType, float scale,
                int leftBoundary, int rightBoundary, float moveSpeed, float attackRange, float retreatDistance, float attackDistance);
void UpdateEnemy2(Enemy2 *e, float heroX);
void RenderEnemy2(SDL_Surface *screen, Enemy2 *e);
void FreeEnemy2(Enemy2 *e);

void InitHero(Hero *hero, int startX);
void UpdateHero(Hero *hero, const Uint8 *keys);
void RenderHero(SDL_Surface *screen, Hero *hero);
void FreeHero(Hero *hero);

void InitMoney(Money *money, TTF_Font *font);
void UpdateMoneyText(Money *money);
void RenderMoney(SDL_Surface *screen, Money *money);
void SaveMoney(Money *money);
void LoadMoney(Money *money);
void FreeMoney(Money *money);

void InitLevel1(Level1 *level);
void UpdateLevel1(Level1 *level, float heroX);
void RenderLevel1(SDL_Surface *screen, Level1 *level);
void FreeLevel1(Level1 *level);

void InitLevel2(Level2 *level);
void UpdateLevel2(Level2 *level, Hero *hero, const Uint8 *keys);
void RenderLevel2(SDL_Surface *screen, Level2 *level);
void FreeLevel2(Level2 *level);

void InitUI(UI *ui, const char *imagePath, SDL_Rect position, float scale, int isHero);
void UpdateUIHealthBar(UI *ui, SDL_Surface *healthBarSurface);
void RenderUI(SDL_Surface *screen, UI *ui);
void FreeUI(UI *ui);

int RectIntersect(SDL_Rect a, SDL_Rect b);
void InitCollisionEffect(CollisionEffect *effect);
void UpdateCollisionEffect(CollisionEffect *effect);
void RenderCollisionEffect(SDL_Surface *screen, CollisionEffect *effect);
void FreeCollisionEffect(CollisionEffect *effect);
void CheckCollisions(Hero *hero, Enemy2 *enemies, int numEnemies, CollisionEffect *effect,
                     Coin *coins, int numCoins, Money *money);

#endif
