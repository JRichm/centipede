#pragma once

#include <SDL3/SDL.h>

#include "constants.h"
#include "bullet.h"
#include "wave_palette.h"


inline SDL_FRect MUSHROOM_FRAME(SDL_Point o, int idx) {
    return { float(o.x + 68 + idx * 9), float(o.y + 72), 8, 8 };
}
inline SDL_FRect MUSHROOM_FRAME_POISONED(SDL_Point o, int idx) {
    return { float(o.x + 68 + idx * 9), float(o.y + 81), 8, 8 };
}


struct Mushroom {
    SDL_FRect rect   = {};
    int       hp     = MUSHROOM_MAX_HP;
    bool      active = true;
    bool      poisoned = false;

    void init(float x, float y) {
        rect     = { x, y, (float)MUSHROOM_W, (float)MUSHROOM_H };
        hp       = MUSHROOM_MAX_HP;
        poisoned = false;
    }

    bool check_bullet(Bullet &b) {
        if (!active || !b.active) return false;
        if (!SDL_HasRectIntersectionFloat(&b.rect, &rect)) return false;
        b.active = false;
        if (--hp <= 0) { active = false; poisoned = false; }
        return true;
    }

    void render(SDL_Renderer *renderer, SDL_Texture *sheet, SDL_Point palette) {
        if (!active) return;
        SDL_FRect src = poisoned
            ? MUSHROOM_FRAME_POISONED(palette, MUSHROOM_MAX_HP - hp)
            : MUSHROOM_FRAME(palette, MUSHROOM_MAX_HP - hp);
        SDL_RenderTexture(renderer, sheet, &src, &rect);
    }
};


struct HealAnimation {
    float x = 0.0f;
    float y = 0.0f;
    int frame = 0;
    float frame_timer = 0.0f;
    bool active = false;
    int mushroom_index = -1;

    SDL_FRect src_rect(SDL_Point palette) const {
        return { float(palette.x + 68 + frame * 17), float(palette.y + 63), 16.0f, 8.0f };
    }

    SDL_FRect dst_rect() const {
        return { x, y, float(MUSHROOM_W), float(MUSHROOM_H) };
    }

    void start(float mx, float my, int idx) {
        x = mx;
        y = my;
        frame = 0;
        frame_timer = HEAL_FRAME_RATE;
        active = true;
        mushroom_index = idx;
    }

    // returns true when animation is complete
    bool update(float dt) {
        if (!active) return false;
        frame_timer -= dt;
        if (frame_timer <= 0.0f) {
            frame++;
            frame_timer = HEAL_FRAME_RATE;
            if (frame >= HEAL_FRAMES) {
                active = false;
                return true;
            }
        }
        return false;
    }

    void render(SDL_Renderer *renderer, SDL_Texture *sheet, SDL_Point palette) {
        if (!active) return;
        SDL_FRect src = src_rect(palette);
        SDL_FRect dst = dst_rect();
        SDL_RenderTexture(renderer, sheet, &src, &dst);
    }
};
