#pragma once

#include <SDL3/SDL.h>

#include "constants.h"
#include "bullet.h"

struct InputState {
    float dx = 0.0f;
    float dy = 0.0f;
    bool fire = false;
};


struct Player {
    SDL_FRect rect     = {};
    float     speed    = PLAYER_SPEED;

    static constexpr SDL_FRect src = { 21.0f, 9.0f, 7.0f, 8.0f };

    bool init() {
        rect.w = PLAYER_W;
        rect.h = PLAYER_H;
        rect.x = (WINDOW_WIDTH - PLAYER_W) / 2.0f;
        rect.y = WINDOW_HEIGHT - PLAYER_H - 16.0f;
        return true;
    }

    void update(const InputState &input, float dt, Bullet &bullet) {
        rect.x += input.dx * speed * dt;
        rect.y += input.dy * speed * dt;
        rect.x = SDL_clamp(rect.x, 0.0f, WINDOW_WIDTH - PLAYER_W);
        rect.y = SDL_clamp(rect.y, PLAYER_ZONE_TOP, WINDOW_HEIGHT - PLAYER_H);
        if (input.fire && !bullet.active)
            bullet.spawn(rect.x, rect.y, rect.w);
    }

    void render(SDL_Renderer *renderer, SDL_Texture *sheet) {
        SDL_RenderTexture(renderer, sheet, &src, &rect);
    }
};
