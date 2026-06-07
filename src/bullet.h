#pragma once

#include <SDL3/SDL.h>

#include "constants.h"

struct Bullet {

    SDL_FRect rect = {};
    bool active = false;


    void spawn(float player_x, float player_y, float player_w) {
        active = true;
        rect.w = BULLET_W;
        rect.h = BULLET_H;
        rect.x = player_x + (player_w - BULLET_W) / 2.0f;
        rect.y = player_y;
    }


    void update(float dt) {
        if (!active) return;
        rect.y -= BULLET_SPEED * dt;
        if (rect.y + rect.h < 0) active = false;
    }


    void render(SDL_Renderer *renderer) {
        if (!active) return;
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
    }
};
