#pragma once

#include <vector>
#include <algorithm>
#include <SDL3/SDL.h>

#include "constants.h"
#include "mushroom.h"
#include "bullet.h"
#include "wave_palette.h"


struct Scorpion {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    bool active = false;
    int frame = 0;
    float frame_timer = 0.0f;

    SDL_FRect rect() const {
        return { x, y, float(SCORPION_W), float(SCORPION_H) };
    }

    SDL_FRect poison_rect() const {
        return {
            x + 1.0f,
            y + 1.0f,
            float(SCORPION_W) - 2.0f,
            float(CELL_PX) - 2.0f
        };
    }

    SDL_FRect src_rect(SDL_Point palette) const { return { float(palette.x + frame * 17), float(palette.y + 72), float(SCORPION_FRAME_W), float(SCORPION_FRAME_H) }; }

    void spawn() {
        active = true;
        frame = 0;
        frame_timer = 0.0f;

        int max_row = int(PLAYER_ZONE_TOP / CELL_PX) - 2;
        int row = 2 + rand() % SDL_max(1, max_row);
        y = float(row * CELL_PX);

        bool from_left = rand() % 2 == 0;
        vx = from_left ? SCORPION_SPEED : -SCORPION_SPEED;
        x  = from_left ? -float(SCORPION_W) : float(WINDOW_WIDTH);
    }

    int check_bullet(Bullet &bullet) {
        if (!active || !bullet.active) return 0;
        SDL_FRect sr = rect();
        if (!SDL_HasRectIntersectionFloat(&bullet.rect, &sr)) return 0;
        bullet.active = false;
        active = false;
        return SCORE_SCORPION;
    }

    bool update(float dt, std::vector<Mushroom> &mushrooms) {
        if (!active) return false;

        frame_timer -= dt;
        if (frame_timer <= 0.0f) {
            frame_timer = SCORPION_FRAME_RATE;
            frame = (frame + 1) % SCORPION_FRAMES;
        }

        x += vx * dt;

        SDL_FRect sr = poison_rect();
        for (auto &m : mushrooms) {
            if (!m.active || m.poisoned) continue;
            if (SDL_HasRectIntersectionFloat(&sr, &m.rect))
                m.poisoned = true;
        }

        if (x + float(SCORPION_W) < 0.0f || x > float(WINDOW_WIDTH)) {
            active = false;
            return true;
        }
        return false;
    }

    void render(SDL_Renderer *renderer, SDL_Texture *sheet, SDL_Point palette) {
        if (!active) return;
        SDL_FRect src = src_rect(palette);
        SDL_FRect dst = rect();

        SDL_FlipMode flip = (vx > 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderTextureRotated(renderer, sheet, &src, &dst, 0.0, nullptr, flip);
    }
};
