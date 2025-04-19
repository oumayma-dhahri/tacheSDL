#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FRAME_DELAY 100
#define SCALE_FACTOR 0.25f
#define HIT_DURATION 500
#define INVINCIBILITY_DURATION 1000
#define MONEY_FILE "money.txt"

int activeCoins;

SDL_Surface* CreateHealthBarSurface(TTF_Font *font, int health, int maxHealth) {
    if (!font) return NULL;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", (health * 100) / maxHealth);
    SDL_Color color = {0, 0, 0, 0};
    return TTF_RenderText_Solid(font, buf, color);
}

SDL_Surface* CreateHeroHealthBarSurface(TTF_Font *font, Hero *hero) {
    if (!font || !hero) return NULL;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", (hero->health * 100) / hero->maxHealth);
    SDL_Color color = {0, 0, 0, 0};
    return TTF_RenderText_Solid(font, buf, color);
}

SDL_Surface* ResizeSurface(SDL_Surface *src, float scale) {
    if (!src) {
        printf("ERREUR: Surface source NULL dans ResizeSurface\n");
        return NULL;
    }
    if (scale <= 0.f) scale = 1.f;
    int newW = (int)(src->w * scale);
    int newH = (int)(src->h * scale);
    if (newW < 1 || newH < 1) return NULL;
    SDL_Surface *dest = SDL_CreateRGBSurface(SDL_SWSURFACE, newW, newH, 32,
                                             0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!dest) {
        printf("ERREUR: Échec création surface dans ResizeSurface\n");
        return NULL;
    }
    for (int y = 0; y < newH; y++) {
        for (int x = 0; x < newW; x++) {
            int sx = (int)(x / scale);
            int sy = (int)(y / scale);
            Uint32 pixel = ((Uint32*)src->pixels)[sy * src->w + sx];
            ((Uint32*)dest->pixels)[y * newW + x] = pixel;
        }
    }
    return dest;
}

void InitCoin(Coin *coin, float startX, float startY) {
    if (!coin) {
        printf("ERREUR: Pointeur Coin NULL dans InitCoin\n");
        exit(EXIT_FAILURE);
    }
    coin->x = startX;
    coin->y = startY;
    coin->initialY = startY;
    coin->active = 1;
    coin->currentFrame = 0;
    coin->frameTimer = SDL_GetTicks();
    coin->frameDelay = FRAME_DELAY;
    coin->speedX = -2.0f;
    coin->amplitude = 50.0f;
    coin->frequency = 0.05f;

    SDL_Surface *img = IMG_Load("assets/coin.png");
    if (!img) {
        printf("ERREUR: Chargement coin.png\n");
        exit(EXIT_FAILURE);
    }
    SDL_Surface *scaledImg = ResizeSurface(img, SCALE_FACTOR);
    SDL_FreeSurface(img);
    if (!scaledImg) {
        printf("ERREUR: Redimensionnement de coin.png\n");
        exit(EXIT_FAILURE);
    }
    coin->spriteSheet = scaledImg;
    coin->totalFrames = TOTAL_FRAMES;
    coin->frames = malloc(sizeof(SDL_Rect) * TOTAL_FRAMES);
    if (!coin->frames) {
        printf("ERREUR: Allocation mémoire pour coin->frames\n");
        SDL_FreeSurface(scaledImg);
        exit(EXIT_FAILURE);
    }
    int frameWidth = (int)(FRAME_WIDTH * SCALE_FACTOR);
    int frameHeight = (int)(FRAME_HEIGHT * SCALE_FACTOR);
    for (int j = 0; j < TOTAL_FRAMES; j++) {
        coin->frames[j].x = j * frameWidth;
        coin->frames[j].y = 0;
        coin->frames[j].w = frameWidth;
        coin->frames[j].h = frameHeight;
    }

    coin->rect.x = (int)startX;
    coin->rect.y = (int)startY;
    coin->rect.w = 30;
    coin->rect.h = 30;
}

void UpdateCoin(Coin *coin) {
    if (!coin) {
        printf("ERREUR: Pointeur Coin NULL dans UpdateCoin\n");
        return;
    }
    if (!coin->active) return;

    coin->x += coin->speedX;
    coin->y = coin->initialY + coin->amplitude * sin(coin->frequency * coin->x);

    if (coin->x < -coin->rect.w) {
        coin->active = 0;
        activeCoins--;
    }

    Uint32 now = SDL_GetTicks();
    if (now - coin->frameTimer >= (Uint32)coin->frameDelay) {
        coin->currentFrame = (coin->currentFrame + 1) % coin->totalFrames;
        coin->frameTimer = now;
    }

    coin->rect.x = (int)coin->x;
    coin->rect.y = (int)coin->y;
}

void RenderCoin(SDL_Surface *screen, Coin *coin) {
    if (!coin) {
        printf("ERREUR: Pointeur Coin NULL dans RenderCoin\n");
        return;
    }
    if (!coin->active) return;

    SDL_Rect src = coin->frames[coin->currentFrame];
    SDL_Rect dst = {(int)coin->x, (int)coin->y, 0, 0};
    SDL_BlitSurface(coin->spriteSheet, &src, screen, &dst);
}

void FreeCoin(Coin *coin) {
    if (!coin) {
        printf("ERREUR: Pointeur Coin NULL dans FreeCoin\n");
        return;
    }
    if (coin->spriteSheet) {
        SDL_FreeSurface(coin->spriteSheet);
        coin->spriteSheet = NULL;
    }
    if (coin->frames) {
        free(coin->frames);
        coin->frames = NULL;
    }
}

void InitEnemy2(Enemy2 *e, const char *spriteSheetPath, int frameWidth, int frameHeight,
                int *framesPerState, int numStates, int maxHealth, int enemyType, float scale,
                int leftBoundary, int rightBoundary, float moveSpeed, float attackRange, float retreatDistance, float attackDistance) {
    if (!e) {
        printf("ERREUR: Pointeur Enemy2 NULL dans InitEnemy2\n");
        exit(EXIT_FAILURE);
    }
    memset(e, 0, sizeof(Enemy2));
    e->x = SCREEN_WIDTH;
    if (enemyType == 0) e->y = getEnemy1Y();
    else if (enemyType == 1) e->y = getEnemy2Y();
    else if (enemyType == 2) e->y = getEnemy3Y();
    else if (enemyType == 3) e->y = getEnemy4Y();
    e->initialY = e->y;
    e->enemyType = enemyType;
    e->direction = -1;
    e->state = ENEMY2_IDLE;
    e->health = maxHealth;
    e->maxHealth = maxHealth;
    e->moveSpeed = moveSpeed;
    e->leftBoundary = leftBoundary;
    e->rightBoundary = rightBoundary;
    e->moving = 1;
    e->moveDirection = -1;
    e->currentFrame = 0;
    e->frameDelay = 100;
    e->frameTimer = SDL_GetTicks();
    e->isFullyVisible = 0;
    e->animationPlaying = 0;
    e->hurtStartTime = 0;
    e->active = 1;
    e->meleeRange = 50.0f;
    e->numStates = numStates;
    e->attackRange = attackRange;
    e->retreatDistance = retreatDistance;
    e->attackTimer = 0;
    e->attackPhase = 0;
    e->attackDistance = attackDistance;

    if (enemyType == 2) {
        e->yMoveActive = 1;
        e->moveCount = 0;
        e->moveDirectionY = -1;
        e->yMoveSpeed = 0.5f;
        e->yTarget = e->initialY - 300.0f;
    }
    if (enemyType == 3) {
        e->sinAmplitude = 100.0f;
        e->sinFrequency = 0.05f;
    }

    printf("DEBUG: Initialisation Enemy2 à (%f, %f) avec %d états, santé %d\n", e->x, e->y, numStates, maxHealth);

    SDL_Surface *img = IMG_Load(spriteSheetPath);
    if (!img) {
        printf("ERREUR: IMG_Load %s\n", spriteSheetPath);
        exit(EXIT_FAILURE);
    }
    SDL_Surface *sheet = ResizeSurface(img, scale);
    SDL_FreeSurface(img);
    if (!sheet) {
        printf("ERREUR: ResizeSurface a retourné NULL pour %s\n", spriteSheetPath);
        exit(EXIT_FAILURE);
    }

    e->animations = malloc(sizeof(Animation) * numStates);
    if (!e->animations) {
        printf("ERREUR: Allocation mémoire pour e->animations\n");
        SDL_FreeSurface(sheet);
        exit(EXIT_FAILURE);
    }

    int scaledFrameWidth = frameWidth * scale;
    int scaledFrameHeight = frameHeight * scale;

    for (int state = 0; state < numStates; state++) {
        e->animations[state].spriteSheet = sheet;
        e->animations[state].totalFrames = framesPerState[state];
        e->animations[state].frames = malloc(sizeof(SDL_Rect) * framesPerState[state]);
        if (!e->animations[state].frames) {
            printf("ERREUR: Allocation mémoire pour e->animations[%d].frames\n", state);
            SDL_FreeSurface(sheet);
            for (int i = 0; i < state; i++) free(e->animations[i].frames);
            free(e->animations);
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < framesPerState[state]; j++) {
            e->animations[state].frames[j].x = j * scaledFrameWidth;
            e->animations[state].frames[j].y = state * scaledFrameHeight;
            e->animations[state].frames[j].w = scaledFrameWidth;
            e->animations[state].frames[j].h = scaledFrameHeight;
        }
    }

    e->rect.w = 200;
    e->rect.h = 200;
    e->rect.x = e->x;
    e->rect.y = e->y;

    printf("DEBUG: InitEnemy2 terminé avec succès\n");
}

void UpdateEnemy2(Enemy2 *e, float heroX) {
    if (!e) {
        printf("ERREUR: Pointeur Enemy2 NULL dans UpdateEnemy2\n");
        return;
    }
    if (!e->active) return;
    Uint32 now = SDL_GetTicks();

    if (e->state < 0 || e->state >= e->numStates) {
        printf("ERREUR: e->state hors limites: %d (max: %d)\n", e->state, e->numStates - 1);
        e->state = ENEMY2_IDLE;
    }

    int framesInState = e->animations[e->state].totalFrames;
    float distanceToHero = fabs(e->x - heroX);

    if (e->enemyType == 2) {
        if (distanceToHero <= 100.0f) {
            e->yMoveActive = 0;
            e->yTarget = e->initialY;
            printf("DEBUG: enemy3 distance <= 100, returning to initialY\n");
        }

        if (e->yMoveActive) {
            if (e->moveDirectionY == -1) {
                e->y -= e->yMoveSpeed;
                if (e->y <= e->yTarget) {
                    e->y = e->yTarget;
                    e->moveDirectionY = 1;
                    e->yTarget = e->initialY;
                }
            } else {
                e->y += e->yMoveSpeed;
                if (e->y >= e->yTarget) {
                    e->y = e->yTarget;
                    e->moveCount++;
                    if (e->moveCount < 4) {
                        e->moveDirectionY = -1;
                        e->yTarget = e->initialY - 300.0f;
                    }
                }
            }

            if (heroX < e->x) {
                e->x -= e->moveSpeed;
                e->direction = -1;
            } else {
                e->x += e->moveSpeed;
                e->direction = 1;
            }
            e->state = ENEMY2_WALK;
            printf("DEBUG: enemy3 in vertical movement, y = %f, distanceToHero = %f\n", e->y, distanceToHero);
        } else {
            if (fabs(e->y - e->initialY) > 1.0f) {
                if (e->y < e->initialY) {
                    e->y += e->yMoveSpeed;
                    if (e->y > e->initialY) e->y = e->initialY;
                } else {
                    e->y -= e->yMoveSpeed;
                    if (e->y < e->initialY) e->y = e->initialY;
                }
                e->state = ENEMY2_WALK;
                printf("DEBUG: enemy3 returning to initialY, y = %f\n", e->y);
            } else {
                if (distanceToHero <= e->attackDistance) {
                    if (e->attackPhase == 0 || e->attackPhase == 3) {
                        e->state = ENEMY2_ATTACK;
                        e->animationPlaying = 1;
                        e->currentFrame = 0;
                        if (e->attackPhase == 3) e->attackPhase = 0;
                        printf("DEBUG: enemy3 attacking hero\n");
                    } else if (e->attackPhase == 1) {
                        e->state = ENEMY2_IDLE;
                        if (now - e->attackTimer >= 500) {
                            e->attackPhase = 2;
                            e->attackTimer = now;
                        }
                    } else if (e->attackPhase == 2) {
                        e->state = ENEMY2_WALK;
                        e->x += e->moveSpeed * e->direction;
                        if (now - e->attackTimer >= 500) {
                            e->attackPhase = 3;
                        }
                    }
                } else {
                    e->state = ENEMY2_WALK;
                    e->x += e->moveSpeed * (heroX < e->x ? -1 : 1);
                    e->direction = (heroX < e->x) ? -1 : 1;
                    printf("DEBUG: enemy3 moving towards hero, x = %f\n", e->x);
                }
            }
        }
    } else {
        if (heroX < e->x) {
            e->direction = -1;
        } else if (heroX > e->x) {
            e->direction = 1;
        }

        if (e->animationPlaying) {
            if (e->currentFrame >= framesInState - 1) {
                e->animationPlaying = 0;
                if (e->state == ENEMY2_HIT) {
                    if (now - e->hurtStartTime >= 500) {
                        e->state = ENEMY2_IDLE;
                        e->currentFrame = 0;
                    }
                } else if (e->state == ENEMY2_DEATH) {
                    e->active = 0;
                } else if (e->state == ENEMY2_ATTACK) {
                    e->state = ENEMY2_IDLE;
                    e->currentFrame = 0;
                    e->attackPhase = 1;
                    e->attackTimer = now;
                }
            }
        } else if (e->state != ENEMY2_DEATH) {
            if (!e->isFullyVisible) {
                e->state = ENEMY2_WALK;
                e->x += e->moveDirection * e->moveSpeed;
                if (e->x <= SCREEN_WIDTH - e->rect.w - 50) e->isFullyVisible = 1;
            } else {
                if (distanceToHero <= e->attackDistance) {
                    if (e->attackPhase == 0 || e->attackPhase == 3) {
                        e->state = ENEMY2_ATTACK;
                        e->animationPlaying = 1;
                        e->currentFrame = 0;
                        if (e->attackPhase == 3) e->attackPhase = 0;
                    } else if (e->attackPhase == 1) {
                        e->state = ENEMY2_IDLE;
                        if (now - e->attackTimer >= 500) {
                            e->attackPhase = 2;
                            e->attackTimer = now;
                        }
                    } else if (e->attackPhase == 2) {
                        e->state = ENEMY2_WALK;
                        e->x += e->moveSpeed * e->direction;
                        if (now - e->attackTimer >= 500) {
                            e->attackPhase = 3;
                        }
                    }
                } else {
                    e->state = ENEMY2_WALK;
                    e->x += e->moveSpeed * (heroX < e->x ? -1 : 1);
                }
            }
        }
        if (e->enemyType == 3 && e->state == ENEMY2_WALK) {
            e->y = e->initialY + e->sinAmplitude * sin(e->sinFrequency * e->x);
        }
    }

    Uint32 effectiveDelay = (e->state == ENEMY2_DEATH) ? 70 : e->frameDelay;
    if (now - e->frameTimer >= effectiveDelay) {
        e->currentFrame++;
        if (e->currentFrame >= framesInState) {
            e->currentFrame = 0;
        }
        e->frameTimer = now;
    }

    e->rect.x = (int)e->x;
    e->rect.y = (int)e->y;
}

void RenderEnemy2(SDL_Surface *screen, Enemy2 *e) {
    if (!e) {
        printf("ERREUR: Pointeur Enemy2 NULL dans RenderEnemy2\n");
        return;
    }
    if (!e->active) return;
    if (e->state < 0 || e->state >= e->numStates) {
        printf("ERREUR: e->state invalide dans RenderEnemy2: %d (max: %d)\n", e->state, e->numStates - 1);
        return;
    }
    int frameIndex = e->currentFrame % e->animations[e->state].totalFrames;
    SDL_Rect src = e->animations[e->state].frames[frameIndex];
    SDL_Rect dst = {e->rect.x, e->rect.y, 0, 0};
    SDL_BlitSurface(e->animations[e->state].spriteSheet, &src, screen, &dst);
}

void FreeEnemy2(Enemy2 *e) {
    if (!e) {
        printf("ERREUR: Pointeur Enemy2 NULL dans FreeEnemy2\n");
        return;
    }
    if (e->animations) {
        if (e->animations[0].spriteSheet) {
            SDL_FreeSurface(e->animations[0].spriteSheet);
            e->animations[0].spriteSheet = NULL;
        }
        for (int i = 0; i < e->numStates; i++) {
            if (e->animations[i].frames) {
                free(e->animations[i].frames);
                e->animations[i].frames = NULL;
            }
        }
        free(e->animations);
        e->animations = NULL;
    }
}

int getHeroY() { return 270; }
int getEnemy1Y() { return 420; }
int getEnemy2Y() { return 500; }
int getEnemy3Y() { return 320; }
int getEnemy4Y() { return 320; }
int getCoinY() { return 370; }

SDL_Rect getHeroUIPos() {
    SDL_Rect pos = {10, 10, 250, 125};
    return pos;
}

SDL_Rect getEnemyUIPos() {
    SDL_Rect pos = {SCREEN_WIDTH - 260, 10, 250, 125};
    return pos;
}

SDL_Rect getHeroHealthTextPos(SDL_Rect uiPos) {
    SDL_Rect pos = {uiPos.x + 145, uiPos.y + 125, 0, 0};
    return pos;
}

SDL_Rect getEnemyHealthTextPos(SDL_Rect uiPos) {
    SDL_Rect pos = {uiPos.x+ 30 , uiPos.y + 125, 0, 0};
    return pos;
}

void InitHero(Hero *hero, int startX) {
    if (!hero) {
        printf("ERREUR: Pointeur Hero NULL dans InitHero\n");
        exit(EXIT_FAILURE);
    }
    memset(hero, 0, sizeof(Hero));
    hero->x = startX;
    hero->y = getHeroY();
    hero->direction = 1;
    hero->state = HERO_IDLE;
    hero->health = 1600;
    hero->maxHealth = 1600;
    hero->moveSpeed = 8.0f;
    hero->frameDelay = FRAME_DELAY;
    hero->isJumping = 0;
    hero->jumpVelocity = -15.0f;
    hero->gravity = 0.6f;
    hero->isAttacking = 0;
    hero->frameTimer = SDL_GetTicks();
    hero->lastUpdateTime = SDL_GetTicks();
    hero->lastHitTime = 0;
    hero->flip = 0;
    hero->animationPlaying = 0;
    hero->attackDamage = 20;

    printf("DEBUG: Initialisation Hero à (%d, %f)\n", startX, hero->y);

    SDL_Surface *whiteSquare = SDL_CreateRGBSurface(0, 400, 400, 32, 0, 0, 0, 0);
    if (!whiteSquare) {
        printf("ERREUR: SDL_CreateRGBSurface\n");
        exit(EXIT_FAILURE);
    }
    SDL_FillRect(whiteSquare, NULL, SDL_MapRGB(whiteSquare->format, 255, 255, 255));

    for (int i = 0; i < 8; i++) {
        hero->animations[i].spriteSheet = whiteSquare;
        hero->animations[i].totalFrames = 1;
        hero->animations[i].frames = malloc(sizeof(SDL_Rect) * 1);
        if (!hero->animations[i].frames) {
            printf("ERREUR: Allocation mémoire échouée pour hero->animations[%d].frames\n", i);
            SDL_FreeSurface(whiteSquare);
            for (int j = 0; j < i; j++) {
                free(hero->animations[j].frames);
            }
            exit(EXIT_FAILURE);
        }
        hero->animations[i].frames[0].x = 0;
        hero->animations[i].frames[0].y = 0;
        hero->animations[i].frames[0].w = 400;
        hero->animations[i].frames[0].h = 400;
    }

    hero->flippedSpriteSheet = NULL;

    hero->rect.w = 400;
    hero->rect.h = 400;
    hero->rect.x = startX;
    hero->rect.y = hero->y;

    printf("DEBUG: InitHero terminé avec succès\n");
}

void UpdateHero(Hero *hero, const Uint8 *keys) {
    if (!hero) {
        printf("ERREUR: Pointeur Hero NULL dans UpdateHero\n");
        return;
    }
    Uint32 now = SDL_GetTicks();
    Uint32 deltaTime = now - hero->lastUpdateTime;
    float speedFactor = deltaTime / 16.666f;

    if (hero->state < 0 || hero->state >= 8) {
        printf("ERREUR: hero->state hors limites: %d (max: 7)\n", hero->state);
        hero->state = HERO_IDLE;
    }

    enum HeroState previousState = hero->state;
    static Uint32 lastAttackTime = 0;

    if (hero->state == HERO_HIT && now - hero->lastHitTime < HIT_DURATION) {
        if (hero->animationPlaying && hero->currentFrame >= hero->animations[HERO_HIT].totalFrames - 1) {
            hero->currentFrame = 0;
        }
    } else if (hero->state == HERO_HIT) {
        hero->animationPlaying = 0;
        hero->state = HERO_IDLE;
        hero->currentFrame = 0;
    } else if (keys[SDLK_a] && now - lastAttackTime >= 500 && !hero->animationPlaying) {
        hero->state = HERO_ATTACK;
        hero->isAttacking = 1;
        hero->currentFrame = 0;
        hero->animationPlaying = 1;
        lastAttackTime = now;
    } else if (hero->animationPlaying) {
        if (hero->currentFrame >= hero->animations[hero->state].totalFrames - 1) {
            hero->animationPlaying = 0;
            if (hero->state == HERO_ATTACK) {
                hero->isAttacking = 0;
                hero->state = HERO_IDLE;
            } else if (hero->state == HERO_DEATH) {
                return;
            }
            hero->currentFrame = 0;
        }
    }

    if (!hero->animationPlaying && hero->state != HERO_DEATH) {
        if (hero->health <= 0) {
            hero->state = HERO_DEATH;
            hero->currentFrame = 0;
            hero->animationPlaying = 1;
        } else {
            if (keys[SDLK_j] && !hero->isJumping) {
                hero->state = HERO_JUMP;
                hero->isJumping = 1;
                hero->jumpVelocity = -15.0f;
                hero->currentFrame = 0;
            } else if (keys[SDLK_SPACE] && keys[SDLK_RIGHT]) {
                hero->state = HERO_RUN;
                hero->direction = 1;
                hero->flip = 0;
                hero->x += hero->moveSpeed * speedFactor * 1.5f;
                if (previousState != HERO_RUN) hero->currentFrame = 0;
            } else if (keys[SDLK_SPACE] && keys[SDLK_LEFT]) {
                hero->state = HERO_RUN;
                hero->direction = -1;
                hero->flip = 1;
                hero->x -= hero->moveSpeed * speedFactor * 1.5f;
                if (previousState != HERO_RUN) hero->currentFrame = 0;
            } else if (keys[SDLK_RIGHT]) {
                hero->state = HERO_WALK;
                hero->direction = 1;
                hero->flip = 0;
                hero->x += hero->moveSpeed * speedFactor;
                if (previousState != HERO_WALK) hero->currentFrame = 0;
            } else if (keys[SDLK_LEFT]) {
                hero->state = HERO_WALK;
                hero->direction = -1;
                hero->flip = 1;
                hero->x -= hero->moveSpeed * speedFactor;
                if (previousState != HERO_WALK) hero->currentFrame = 0;
            } else {
                hero->state = HERO_IDLE;
                if (previousState != HERO_IDLE) hero->currentFrame = 0;
            }
        }
    }

    if (hero->isJumping) {
        hero->y += hero->jumpVelocity * speedFactor;
        hero->jumpVelocity += hero->gravity * speedFactor;
        if (hero->y >= getHeroY()) {
            hero->y = getHeroY();
            hero->isJumping = 0;
            hero->state = HERO_IDLE;
            hero->currentFrame = 0;
        } else if (hero->jumpVelocity > 0) {
            hero->state = HERO_FALL;
            if (previousState != HERO_FALL) hero->currentFrame = 0;
        }
    }

    if (hero->x < 0) hero->x = 0;
    if (hero->x > SCREEN_WIDTH - hero->rect.w) hero->x = SCREEN_WIDTH - hero->rect.w;
    if (hero->y < 0) hero->y = 0;
    if (hero->y > SCREEN_HEIGHT - hero->rect.h) hero->y = SCREEN_HEIGHT - hero->rect.h;

    hero->rect.x = (int)hero->x;
    hero->rect.y = (int)hero->y;

    if (hero->animations[hero->state].totalFrames > 0 && now - hero->frameTimer >= hero->frameDelay) {
        hero->currentFrame++;
        if (hero->currentFrame >= hero->animations[hero->state].totalFrames) {
            hero->currentFrame = 0;
        }
        hero->frameTimer = now;
    }

    hero->lastUpdateTime = now;
}

void RenderHero(SDL_Surface *screen, Hero *hero) {
    if (!hero) {
        printf("ERREUR: Pointeur Hero NULL dans RenderHero\n");
        return;
    }

    //ken 3inek tne7i carre blanche ne7i fonction renderhero
    Animation *anim = &hero->animations[hero->state];
    if (anim->totalFrames <= 0) {
        printf("ERREUR: Aucune frame pour l'état %d dans RenderHero\n", hero->state);
        return;
    }
    int frameIndex = hero->currentFrame % anim->totalFrames;
    SDL_Rect src = anim->frames[frameIndex];
    SDL_Rect dst = {hero->rect.x, hero->rect.y, 0, 0};

    if (hero->state == HERO_HIT) {
        printf("DEBUG: RenderHero HERO_HIT, frame=%d, src=(%d,%d,%d,%d), dst=(%d,%d)\n",
               frameIndex, src.x, src.y, src.w, src.h, dst.x, dst.y);
    }

    SDL_Surface *currentSheet = anim->spriteSheet;
    if (!currentSheet) {
        printf("ERREUR: SpriteSheet NULL pour état %d dans RenderHero\n", hero->state);
        return;
    }
    SDL_BlitSurface(currentSheet, &src, screen, &dst);
    
}

void FreeHero(Hero *hero) {
    if (!hero) {
        printf("ERREUR: Pointeur Hero NULL dans FreeHero\n");
        return;
    }
    if (hero->animations[0].spriteSheet) {
        SDL_FreeSurface(hero->animations[0].spriteSheet);
        hero->animations[0].spriteSheet = NULL;
    }
    for (int i = 0; i < 8; i++) {
        if (hero->animations[i].frames) {
            free(hero->animations[i].frames);
            hero->animations[i].frames = NULL;
        }
    }
}

void InitMoney(Money *money, TTF_Font *font) {
    if (!money || !font) {
        printf("ERREUR: Pointeur NULL dans InitMoney\n");
        return;
    }
    money->count = 0;
    money->font = font;
    money->textSurface = NULL;
    money->position.x = SCREEN_WIDTH - 150;
    money->position.y = 10;
    LoadMoney(money);
    UpdateMoneyText(money);
}

void UpdateMoneyText(Money *money) {
    if (!money || !money->font) {
        printf("ERREUR: Pointeur NULL dans UpdateMoneyText\n");
        return;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "Coins: %d", money->count);
    SDL_Color color = {255, 255, 255, 0};
    if (money->textSurface) {
        SDL_FreeSurface(money->textSurface);
        money->textSurface = NULL;
    }
    money->textSurface = TTF_RenderText_Solid(money->font, buf, color);
    if (!money->textSurface) {
        printf("ERREUR: Échec de TTF_RenderText_Solid dans UpdateMoneyText\n");
    }
}

void RenderMoney(SDL_Surface *screen, Money *money) {
    if (!screen || !money) {
        printf("ERREUR: Pointeur NULL dans RenderMoney\n");
        return;
    }
    if (money->textSurface) {
        SDL_BlitSurface(money->textSurface, NULL, screen, &money->position);
    }
}

void SaveMoney(Money *money) {
    if (!money) {
        printf("ERREUR: Pointeur NULL dans SaveMoney\n");
        return;
    }
    FILE *file = fopen(MONEY_FILE, "w");
    if (file) {
        fprintf(file, "%d\n", money->count);
        fclose(file);
    } else {
        printf("ERREUR: Impossible d'ouvrir %s pour écriture\n", MONEY_FILE);
    }
}

void LoadMoney(Money *money) {
    if (!money) {
        printf("ERREUR: Pointeur NULL dans LoadMoney\n");
        return;
    }
    FILE *file = fopen(MONEY_FILE, "r");
    if (file) {
        if (fscanf(file, "%d", &money->count) != 1) {
            printf("ERREUR: Échec de lecture dans %s\n", MONEY_FILE);
            money->count = 0;
        }
        fclose(file);
    } else {
        printf("AVERTISSEMENT: %s n'existe pas, count reste à %d\n", MONEY_FILE, money->count);
    }
}

void FreeMoney(Money *money) {
    if (!money) {
        printf("ERREUR: Pointeur NULL dans FreeMoney\n");
        return;
    }
    if (money->textSurface) {
        SDL_FreeSurface(money->textSurface);
        money->textSurface = NULL;
    }
}

void InitLevel1(Level1 *level) {
    if (!level) {
        printf("ERREUR: Pointeur Level1 NULL dans InitLevel1\n");
        exit(EXIT_FAILURE);
    }
    level->background = IMG_Load("assets/bg.jpg");
    if (!level->background) {
        printf("ERREUR: Chargement bg.jpg\n");
        exit(EXIT_FAILURE);
    }
    int framesPerState[] = {10, 10, 10, 10, 10};
    InitEnemy2(&level->enemies[0], "assets/enemy1.png", 300, 300, framesPerState, 5, 80, 0, 1.0f, 0, SCREEN_WIDTH, 4.0f, 100.0f, 50.0f, 150.0f);
    InitEnemy2(&level->enemies[1], "assets/enemy2.png", 300, 300, framesPerState, 5, 70, 1, 1.0f, 0, SCREEN_WIDTH, 4.0f, 100.0f, 50.0f, 150.0f);
    level->enemies[1].active = 0;
}

void UpdateLevel1(Level1 *level, float heroX) {
    if (!level) {
        printf("ERREUR: Pointeur Level1 NULL dans UpdateLevel1\n");
        return;
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (level->enemies[i].active) {
            UpdateEnemy2(&level->enemies[i], heroX);
        }
    }
    if (!level->enemies[0].active && !level->enemies[1].active) {
        float randVal = (float)rand() / RAND_MAX;
        int enemyToActivate = (randVal < 0.7) ? 0 : 1;
        level->enemies[enemyToActivate].active = 1;
        level->enemies[enemyToActivate].x = 1600;
        level->enemies[enemyToActivate].y = (level->enemies[enemyToActivate].enemyType == 0) ? getEnemy1Y() : getEnemy2Y();
        level->enemies[enemyToActivate].direction = -1;
        level->enemies[enemyToActivate].state = ENEMY2_IDLE;
        level->enemies[enemyToActivate].health = level->enemies[enemyToActivate].maxHealth;
        level->enemies[enemyToActivate].currentFrame = 0;
        level->enemies[enemyToActivate].frameTimer = SDL_GetTicks();
        level->enemies[enemyToActivate].isFullyVisible = 0;
        level->enemies[enemyToActivate].animationPlaying = 0;
        level->enemies[enemyToActivate].hurtStartTime = 0;
        level->enemies[enemyToActivate].attackTimer = 0;
        level->enemies[enemyToActivate].attackPhase = 0;
    }
}

void RenderLevel1(SDL_Surface *screen, Level1 *level) {
    if (!screen || !level) {
        printf("ERREUR: Pointeur NULL dans RenderLevel1\n");
        return;
    }
    SDL_BlitSurface(level->background, NULL, screen, NULL);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (level->enemies[i].active) {
            RenderEnemy2(screen, &level->enemies[i]);
        }
    }
}

void FreeLevel1(Level1 *level) {
    if (!level) {
        printf("ERREUR: Pointeur NULL dans FreeLevel1\n");
        return;
    }
    if (level->background) {
        SDL_FreeSurface(level->background);
        level->background = NULL;
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        FreeEnemy2(&level->enemies[i]);
    }
}

void InitLevel2(Level2 *level) {
    if (!level) {
        printf("ERREUR: Pointeur Level2 NULL dans InitLevel2\n");
        exit(EXIT_FAILURE);
    }
    level->background = IMG_Load("assets/bg2.png");
    if (!level->background) {
        printf("ERREUR: Chargement bg2.png\n");
        exit(EXIT_FAILURE);
    }
    int framesPerState[] = {10, 10, 10, 10, 10};
    InitEnemy2(&level->enemies[0], "assets/enemy3.png", 300, 300, framesPerState, 5, 80, 2, 1.0f, 0, SCREEN_WIDTH, 4.0f, 100.0f, 50.0f, 150.0f);
    InitEnemy2(&level->enemies[1], "assets/enemy4.png", 300, 300, framesPerState, 5, 80, 3, 1.0f, 0, SCREEN_WIDTH, 4.0f, 100.0f, 50.0f, 150.0f);
    level->enemies[0].active = 0;
    level->enemies[1].active = 0;
    level->currentEnemy = 0;
    printf("DEBUG: Level2 initialisé, enemy3 (idx 0, santé=80) et enemy4 (idx 1, santé=80) prêts\n");
}

void UpdateLevel2(Level2 *level, Hero *hero, const Uint8 *keys) {
    if (!level || !hero || !keys) {
        printf("ERREUR: Pointeur NULL dans UpdateLevel2\n");
        return;
    }

    if (keys[SDLK_x]) {
        int current = level->currentEnemy;
        int next = 1 - current;
        if (level->enemies[current].active) {
            level->enemies[current].active = 0;
            printf("DEBUG: Ennemi %d (Type %d) désactivé via 'x'\n", current, level->enemies[current].enemyType);
        }
        if (!level->enemies[next].active) {
            level->enemies[next].active = 1;
            level->enemies[next].x = SCREEN_WIDTH;
            level->enemies[next].y = (level->enemies[next].enemyType == 2) ? getEnemy3Y() : getEnemy4Y();
            level->enemies[next].initialY = level->enemies[next].y;
            level->enemies[next].direction = -1;
            level->enemies[next].state = ENEMY2_WALK;
            level->enemies[next].health = level->enemies[next].maxHealth;
            level->enemies[next].currentFrame = 0;
            level->enemies[next].frameTimer = SDL_GetTicks();
            level->enemies[next].isFullyVisible = 0;
            level->enemies[next].animationPlaying = 0;
            level->enemies[next].hurtStartTime = 0;
            level->enemies[next].attackTimer = 0;
            level->enemies[next].attackPhase = 0;
            if (level->enemies[next].enemyType == 2) {
                level->enemies[next].yMoveActive = 1;
                level->enemies[next].moveCount = 0;
                level->enemies[next].moveDirectionY = -1;
                level->enemies[next].yTarget = level->enemies[next].initialY - 300.0f;
            }
            printf("DEBUG: Ennemi %d (Type %d) activé via 'x'\n", next, level->enemies[next].enemyType);
            level->currentEnemy = next;
        }
    }

    if (keys[SDLK_LSHIFT] || keys[SDLK_RSHIFT]) {
        if (!level->enemies[0].active && !level->enemies[1].active) {
            int idx = level->currentEnemy;
            level->enemies[idx].active = 1;
            level->enemies[idx].x = SCREEN_WIDTH;
            level->enemies[idx].y = (level->enemies[idx].enemyType == 2) ? getEnemy3Y() : getEnemy4Y();
            level->enemies[idx].initialY = level->enemies[idx].y;
            level->enemies[idx].direction = -1;
            level->enemies[idx].state = ENEMY2_WALK;
            level->enemies[idx].health = level->enemies[idx].maxHealth;
            level->enemies[idx].currentFrame = 0;
            level->enemies[idx].frameTimer = SDL_GetTicks();
            level->enemies[idx].isFullyVisible = 0;
            level->enemies[idx].animationPlaying = 0;
            level->enemies[idx].hurtStartTime = 0;
            level->enemies[idx].attackTimer = 0;
            level->enemies[idx].attackPhase = 0;
            if (level->enemies[idx].enemyType == 2) {
                level->enemies[idx].yMoveActive = 1;
                level->enemies[idx].moveCount = 0;
                level->enemies[idx].moveDirectionY = -1;
                level->enemies[idx].yTarget = level->enemies[idx].initialY - 300.0f;
            }
            printf("DEBUG: Ennemi %d (Type %d) activé via Shift\n", idx, level->enemies[idx].enemyType);
        }
    }
    if (level->enemies[level->currentEnemy].active) {
        UpdateEnemy2(&level->enemies[level->currentEnemy], hero->x);
        if (!level->enemies[level->currentEnemy].active) {
            int nextEnemy = 1 - level->currentEnemy;
            level->enemies[nextEnemy].active = 1;
            level->enemies[nextEnemy].x = SCREEN_WIDTH;
            level->enemies[nextEnemy].y = (level->enemies[nextEnemy].enemyType == 2) ? getEnemy3Y() : getEnemy4Y();
            level->enemies[nextEnemy].initialY = level->enemies[nextEnemy].y;
            level->enemies[nextEnemy].direction = -1;
            level->enemies[nextEnemy].state = ENEMY2_WALK;
            level->enemies[nextEnemy].health = level->enemies[nextEnemy].maxHealth;
            level->enemies[nextEnemy].currentFrame = 0;
            level->enemies[nextEnemy].frameTimer = SDL_GetTicks();
            level->enemies[nextEnemy].isFullyVisible = 0;
            level->enemies[nextEnemy].animationPlaying = 0;
            level->enemies[nextEnemy].hurtStartTime = 0;
            level->enemies[nextEnemy].attackTimer = 0;
            level->enemies[nextEnemy].attackPhase = 0;
            if (level->enemies[nextEnemy].enemyType == 2) {
                level->enemies[nextEnemy].yMoveActive = 1;
                level->enemies[nextEnemy].moveCount = 0;
                level->enemies[nextEnemy].moveDirectionY = -1;
                level->enemies[nextEnemy].yTarget = level->enemies[nextEnemy].initialY - 300.0f;
            }
            printf("DEBUG: Ennemi %d (Type %d) désactivé, activation de l'ennemi %d (Type %d)\n",
                   level->currentEnemy, level->enemies[level->currentEnemy].enemyType,
                   nextEnemy, level->enemies[nextEnemy].enemyType);
            level->currentEnemy = nextEnemy;
        }
    }
}

void RenderLevel2(SDL_Surface *screen, Level2 *level) {
    if (!screen || !level) {
        printf("ERREUR: Pointeur NULL dans RenderLevel2\n");
        return;
    }
    SDL_BlitSurface(level->background, NULL, screen, NULL);
    for (int i = 0; i < 2; i++) {
        if (level->enemies[i].active) {
            RenderEnemy2(screen, &level->enemies[i]);
        }
    }
}

void FreeLevel2(Level2 *level) {
    if (!level) {
        printf("ERREUR: Pointeur NULL dans FreeLevel2\n");
        return;
    }
    if (level->background) {
        SDL_FreeSurface(level->background);
        level->background = NULL;
    }
    for (int i = 0; i < 2; i++) {
        FreeEnemy2(&level->enemies[i]);
    }
}

void InitUI(UI *ui, const char *imagePath, SDL_Rect position, float scale, int isHero) {
    if (!ui) {
        printf("ERREUR: Pointeur UI NULL dans InitUI\n");
        exit(EXIT_FAILURE);
    }
    ui->image = NULL;
    ui->position = position;
    ui->healthBar = NULL;
    if (isHero) {
        ui->healthBarPos = getHeroHealthTextPos(position);
    } else {
        ui->healthBarPos = getEnemyHealthTextPos(position);
    }
}

void UpdateUIHealthBar(UI *ui, SDL_Surface *healthBarSurface) {
    if (!ui) {
        printf("ERREUR: Pointeur UI NULL dans UpdateUIHealthBar\n");
        return;
    }
    if (ui->healthBar) {
        SDL_FreeSurface(ui->healthBar);
    }
    ui->healthBar = healthBarSurface;
}

void RenderUI(SDL_Surface *screen, UI *ui) {
    if (!screen || !ui) {
        printf("ERREUR: Pointeur NULL dans RenderUI\n");
        return;
    }
    if (ui->healthBar) {
        SDL_BlitSurface(ui->healthBar, NULL, screen, &ui->healthBarPos);
    }
}

void FreeUI(UI *ui) {
    if (!ui) {
        printf("ERREUR: Pointeur UI NULL dans FreeUI\n");
        return;
    }
    if (ui->healthBar) {
        SDL_FreeSurface(ui->healthBar);
        ui->healthBar = NULL;
    }
}

int RectIntersect(SDL_Rect a, SDL_Rect b) {
    if (a.x + a.w <= b.x || b.x + b.w <= a.x) return 0;
    if (a.y + a.h <= b.y || b.y + b.h <= a.y) return 0;
    return 1;
}

void InitCollisionEffect(CollisionEffect *effect) {
    if (!effect) {
        printf("ERREUR: Pointeur CollisionEffect NULL dans InitCollisionEffect\n");
        exit(EXIT_FAILURE);
    }
    effect->active = 0;
    effect->spriteSheet = IMG_Load("assets/collision.png");
    if (!effect->spriteSheet) {
        printf("ERREUR: Chargement collision.png\n");
        exit(EXIT_FAILURE);
    }
    int frameWidth = 300;
    int frameHeight = 300;
    for (int row = 0; row < 3; row++) {
        effect->animations[row].spriteSheet = effect->spriteSheet;
        effect->animations[row].totalFrames = 7;
        effect->animations[row].frames = malloc(sizeof(SDL_Rect) * 7);
        if (!effect->animations[row].frames) {
            printf("ERREUR: Allocation mémoire échouée pour effect->animations[%d].frames\n", row);
            exit(EXIT_FAILURE);
        }
        for (int col = 0; col < 7; col++) {
            effect->animations[row].frames[col].x = col * frameWidth;
            effect->animations[row].frames[col].y = row * frameHeight;
            effect->animations[row].frames[col].w = frameWidth;
            effect->animations[row].frames[col].h = frameHeight;
        }
    }
    effect->frameDelay = 50;
    effect->currentAnimation = 0;
    printf("DEBUG: InitCollisionEffect terminé avec succès\n");
}

void UpdateCollisionEffect(CollisionEffect *effect) {
    if (!effect) {
        printf("ERREUR: Pointeur CollisionEffect NULL dans UpdateCollisionEffect\n");
        return;
    }
    Uint32 now = SDL_GetTicks();
    if (effect->active && now - effect->frameTimer >= effect->frameDelay) {
        effect->currentFrame++;
        if (effect->currentFrame >= effect->animations[effect->currentAnimation].totalFrames) {
            effect->active = 0;
        }
        effect->frameTimer = now;
    }
}

void RenderCollisionEffect(SDL_Surface *screen, CollisionEffect *effect) {
    if (!effect) {
        printf("ERREUR: Pointeur CollisionEffect NULL dans RenderCollisionEffect\n");
        return;
    }
    if (effect->active) {
        Animation *anim = &effect->animations[effect->currentAnimation];
        SDL_Rect src = anim->frames[effect->currentFrame];
        SDL_Rect dst = {(int)effect->x, (int)effect->y, 0, 0};
        SDL_BlitSurface(anim->spriteSheet, &src, screen, &dst);
    }
}

void FreeCollisionEffect(CollisionEffect *effect) {
    if (!effect) {
        printf("ERREUR: Pointeur CollisionEffect NULL dans FreeCollisionEffect\n");
        return;
    }
    if (effect->spriteSheet) {
        SDL_FreeSurface(effect->spriteSheet);
        effect->spriteSheet = NULL;
    }
    for (int i = 0; i < 3; i++) {
        if (effect->animations[i].frames) {
            free(effect->animations[i].frames);
            effect->animations[i].frames = NULL;
        }
    }
}

void CheckCollisions(Hero *hero, Enemy2 *enemies, int numEnemies, CollisionEffect *effect,
                     Coin *coins, int numCoins, Money *money) {
    if (!hero || !enemies || !effect || !coins || !money) {
        printf("ERREUR: Pointeur NULL dans CheckCollisions\n");
        return;
    }

    Uint32 now = SDL_GetTicks();

    for (int i = 0; i < numEnemies; i++) {
        if (enemies[i].active && enemies[i].health > 0) {
            SDL_Rect heroCollisionRect = {
                hero->rect.x + 25, hero->rect.y + 25,
                hero->rect.w - 50, hero->rect.h - 50
            };
            SDL_Rect enemyCollisionRect = {
                enemies[i].rect.x + 10, enemies[i].rect.y + 10,
                enemies[i].rect.w - 20, enemies[i].rect.h - 20
            };

            printf("DEBUG: Hero rect=(%d,%d,%d,%d), Enemy %d rect=(%d,%d,%d,%d), Type=%d\n",
                   heroCollisionRect.x, heroCollisionRect.y, heroCollisionRect.w, heroCollisionRect.h,
                   i, enemyCollisionRect.x, enemyCollisionRect.y, enemyCollisionRect.w, enemyCollisionRect.h,
                   enemies[i].enemyType);

            if (RectIntersect(heroCollisionRect, enemyCollisionRect)) {
                printf("DEBUG: Collision détectée entre héros et ennemi %d (Type %d)\n", i, enemies[i].enemyType);
                if (hero->isAttacking && hero->state == HERO_ATTACK) {
                    enemies[i].health -= hero->attackDamage;
                    printf("DEBUG: Héros attaque ennemi %d (Type %d), santé restante = %d\n", i, enemies[i].enemyType, enemies[i].health);
                    if (enemies[i].health <= 0) {
                        enemies[i].state = ENEMY2_DEATH;
                        enemies[i].animationPlaying = 1;
                        printf("DEBUG: Ennemi %d (Type %d) passe à ENEMY2_DEATH\n", i, enemies[i].enemyType);
                    } else if (enemies[i].state != ENEMY2_HIT) {
                        enemies[i].state = ENEMY2_HIT;
                        enemies[i].animationPlaying = 1;
                        enemies[i].hurtStartTime = now;
                        printf("DEBUG: Ennemi %d (Type %d) touché\n", i, enemies[i].enemyType);
                    }
                    enemies[i].currentFrame = 0;
                    enemies[i].frameTimer = now;
                    effect->currentAnimation = rand() % 3;
                    effect->active = 1;
                    effect->x = (hero->rect.x + enemies[i].rect.x) / 2;
                    effect->y = (hero->rect.y + enemies[i].rect.y) / 2;
                    effect->currentFrame = 0;
                    effect->frameTimer = now;
                }
                else if (enemies[i].state == ENEMY2_ATTACK && now - hero->lastHitTime >= INVINCIBILITY_DURATION) {
                    hero->health -= 10;
                    hero->lastHitTime = now;
                    printf("DEBUG: Ennemi %d (Type %d) attaque héros, santé héros = %d\n", i, enemies[i].enemyType, hero->health);
                    if (hero->health <= 0) {
                        hero->state = HERO_DEATH;
                        hero->animationPlaying = 1;
                        printf("DEBUG: Héros mort\n");
                    } else if (hero->state != HERO_HIT) {
                        hero->state = HERO_HIT;
                        hero->animationPlaying = 1;
                        hero->currentFrame = 0;
                        printf("DEBUG: Héros touché\n");
                    }
                    hero->frameTimer = now;
                    effect->currentAnimation = rand() % 3;
                    effect->active = 1;
                    effect->x = (hero->rect.x + enemies[i].rect.x) / 2;
                    effect->y = (hero->rect.y + enemies[i].rect.y) / 2;
                    effect->currentFrame = 0;
                    effect->frameTimer = now;
                }
            }
        }
    }

    for (int i = 0; i < numCoins; i++) {
        if (coins[i].active) {
            SDL_Rect heroCollisionRect = {
                hero->rect.x + 25, hero->rect.y + 25,
                hero->rect.w - 50, hero->rect.h - 50
            };
            if (RectIntersect(heroCollisionRect, coins[i].rect)) {
                printf("DEBUG: Pièce %d collectée\n", i);
                coins[i].active = 0;
                activeCoins--;
                money->count++;
                UpdateMoneyText(money);
                SaveMoney(money);
            }
        }
    }
}
