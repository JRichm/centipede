#pragma once

#include <SDL3/SDL.h>

#include "constants.h"
#include "bullet.h"
#include "wave_palette.h"

struct InputState {
    float dx = 0.0f;
    float dy = 0.0f;
    bool fire = false;
};


struct Player {
    SDL_FRect rect     = {};
    float     speed    = PLAYER_SPEED;

    static SDL_FRect player_src(SDL_Point palette) {
        return { float(palette.x + 21), float(palette.y + 9), 7.0f, 8.0f };
    }

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

    void render(SDL_Renderer *renderer, SDL_Texture *sheet, SDL_Point palette) {
        SDL_FRect s = player_src(palette);
        SDL_RenderTexture(renderer, sheet, &s, &rect);
    }
};
