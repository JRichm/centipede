#pragma once

#include <SDL3/SDL.h>

#include "constants.h"
#include "bullet.h"


static constexpr SDL_FRect MUSHROOM_FRAMES[MUSHROOM_MAX_HP] = {
    { 68, 72, 8, 8 },
    { 77, 72, 8, 8 },
    { 86, 72, 8, 8 },
    { 95, 72, 8, 8 },
};


static constexpr SDL_FRect MUSHROOM_FRAMES_POISONED[MUSHROOM_MAX_HP] = {
    { 68, 81, 8, 8 },
    { 77, 81, 8, 8 },
    { 86, 81, 8, 8 },
    { 95, 81, 8, 8 },
};


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

    void render(SDL_Renderer *renderer, SDL_Texture *sheet) {
        if (!active) return;
        const SDL_FRect *src = poisoned
            ? &MUSHROOM_FRAMES_POISONED[MUSHROOM_MAX_HP - hp]
            : &MUSHROOM_FRAMES[MUSHROOM_MAX_HP - hp];
        SDL_RenderTexture(renderer, sheet, src, &rect);
    }
};


struct HealAnimation {
    float x = 0.0f;
    float y = 0.0f;
    int frame = 0;
    float frame_timer = 0.0f;
    bool active = false;
    int mushroom_index = -1;

    SDL_FRect src_rect() const {
        return { 68.0f + float(frame * 17), 63.0f, 16.0f, 8.0f };
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

    void render(SDL_Renderer *renderer, SDL_Texture *sheet) {
        if (!active) return;
        SDL_FRect src = src_rect();
        SDL_FRect dst = dst_rect();
        SDL_RenderTexture(renderer, sheet, &src, &dst);
    }
};
