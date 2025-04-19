#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FRAME_TIME (1000/60)
#define COIN_SPAWN_INTERVAL 1000

int activeCoins;

int main(void) {
    srand(time(NULL));
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
        printf("ERREUR: Initialisation SDL/TTF\n");
        return 1;
    }
    SDL_Surface *screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
    if (!screen) {
        printf("ERREUR: SDL_SetVideoMode\n");
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("assets/arial.ttf", 24);
    if (!font) {
        printf("ERREUR: Chargement arial.ttf\n");
        SDL_Quit();
        TTF_Quit();
        return 1;
    }

    Hero hero;
    InitHero(&hero, 100);

    Level1 level1;
    InitLevel1(&level1);

    Level2 level2;
    InitLevel2(&level2);

    CollisionEffect collisionEffect;
    InitCollisionEffect(&collisionEffect);

    Coin coins[MAX_COINS];
    for (int i = 0; i < MAX_COINS; i++) {
        InitCoin(&coins[i], SCREEN_WIDTH + i * 200, getCoinY());
        coins[i].active = 0;
    }
    Uint32 lastCoinSpawn = 0;
    activeCoins = 0;

    Money money;
    InitMoney(&money, font);

    UI heroUI;
    InitUI(&heroUI, "assets/ui/heroui.png", getHeroUIPos(), 0.25f, 1);
    UI enemy1UI;
    InitUI(&enemy1UI, "assets/ui/enemy1ui.png", getEnemyUIPos(), 0.25f, 0);
    UI enemy2UI;
    InitUI(&enemy2UI, "assets/ui/enemy2ui.png", getEnemyUIPos(), 0.25f, 0);

    int currentLevel = 0;
    int running = 1;
    while (running) {
        Uint32 frameStart = SDL_GetTicks();

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT || (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)) {
                running = 0;
            }
        }

        const Uint8 *keys = SDL_GetKeyState(NULL);
        if (currentLevel == 0 && (keys[SDLK_LSHIFT] || keys[SDLK_RSHIFT])) {
            currentLevel = 1;
        }

        UpdateHero(&hero, keys);
        if (currentLevel == 0) {
            UpdateLevel1(&level1, hero.x);
        } else {
            UpdateLevel2(&level2, &hero, keys);
        }

        Uint32 now = SDL_GetTicks();
        if (now - lastCoinSpawn >= COIN_SPAWN_INTERVAL && activeCoins < 5) {
            for (int i = 0; i < MAX_COINS; i++) {
                if (!coins[i].active) {
                    InitCoin(&coins[i], SCREEN_WIDTH, getCoinY());
                    coins[i].active = 1;
                    activeCoins++;
                    lastCoinSpawn = now;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_COINS; i++) {
            UpdateCoin(&coins[i]);
        }

        if (currentLevel == 0) {
            CheckCollisions(&hero, level1.enemies, MAX_ENEMIES, &collisionEffect, coins, MAX_COINS, &money);
        } else {
            CheckCollisions(&hero, level2.enemies, 2, &collisionEffect, coins, MAX_COINS, &money);
        }
        UpdateCollisionEffect(&collisionEffect);

        if (currentLevel == 0) {
            RenderLevel1(screen, &level1);
        } else {
            RenderLevel2(screen, &level2);
        }
        RenderHero(screen, &hero);
        for (int i = 0; i < MAX_COINS; i++) {
            RenderCoin(screen, &coins[i]);
        }
        RenderCollisionEffect(screen, &collisionEffect);

        UpdateUIHealthBar(&heroUI, CreateHeroHealthBarSurface(font, &hero));
        RenderUI(screen, &heroUI);

        if (currentLevel == 0) {
            int activeEnemyIndex = -1;
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (level1.enemies[i].active) {
                    activeEnemyIndex = i;
                    break;
                }
            }
            if (activeEnemyIndex != -1) {
                UI *currentEnemyUI = (level1.enemies[activeEnemyIndex].enemyType == 0) ? &enemy1UI : &enemy2UI;
                UpdateUIHealthBar(currentEnemyUI, CreateHealthBarSurface(font, level1.enemies[activeEnemyIndex].health, level1.enemies[activeEnemyIndex].maxHealth));
                RenderUI(screen, currentEnemyUI);
            }
        } else {
            for (int i = 0; i < 2; i++) {
                if (level2.enemies[i].active) {
                    UI *currentEnemyUI = (level2.enemies[i].enemyType == 2) ? &enemy2UI : &enemy1UI;
                    UpdateUIHealthBar(currentEnemyUI, CreateHealthBarSurface(font, level2.enemies[i].health, level2.enemies[i].maxHealth));
                    RenderUI(screen, currentEnemyUI);
                    break;
                }
            }
        }

        RenderMoney(screen, &money);
        SDL_Flip(screen);

        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < FRAME_TIME) SDL_Delay(FRAME_TIME - frameTime);
    }

    FreeHero(&hero);
    FreeLevel1(&level1);
    FreeLevel2(&level2);
    FreeCollisionEffect(&collisionEffect);
    for (int i = 0; i < MAX_COINS; i++) {
        FreeCoin(&coins[i]);
    }
    FreeMoney(&money);
    FreeUI(&heroUI);
    FreeUI(&enemy1UI);
    FreeUI(&enemy2UI);

    TTF_CloseFont(font);
    SDL_FreeSurface(screen);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
