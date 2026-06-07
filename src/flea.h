#pragma once

#include <vector>
#include <algorithm>
#include <SDL3/SDL.h>

#include "constants.h"
#include "bullet.h"
#include "wave_palette.h"
 

struct Flea {
    float x = 0.0f;
    float y = 0.0f;
    float speed = FLEA_SPEED;
    bool  active = false;
    int   hp = 2;
    int   frame = 0;
    float frame_timer = 0.0f;

    std::vector<float> drop_ys;
    int drop_index = 0;

    SDL_FRect rect() const {
        return { x, y, float(FLEA_W), float(FLEA_H) };
    }

    SDL_FRect src_rect(SDL_Point palette) const {
        return {
            float(palette.x + frame * 17),
            float(palette.y + 63),
            float(FLEA_FRAME_W),
            float(FLEA_FRAME_H) };
        }

    void spawn() {
        active = true;
        hp = 2;
        speed = FLEA_SPEED;
        frame = 0;
        frame_timer = 0.0f;
        drop_index = 0;
        drop_ys.clear();

        int col = rand() % GRID_COLS;
        x = float(col * CELL_PX);
        y = -float(FLEA_H);

        int count = FLEA_MUSHROOM_MIN + rand() % (FLEA_MUSHROOM_MAX - FLEA_MUSHROOM_MIN + 1);

        float spacing = float(WINDOW_HEIGHT) / float(count + 1);
        for (int i = 0; i < count; i++) {
            float base_y = spacing * float(i + 1);
            float offset = float((rand() % (int)spacing) - (int)spacing / 2);
            float drop_y = SDL_clamp(base_y + offset, 0.0f, float(WINDOW_HEIGHT - CELL_PX));
            drop_y = float(int(drop_y / CELL_PX) * CELL_PX);
            drop_ys.push_back(drop_y);
        }

        std::sort(drop_ys.begin(), drop_ys.end());
    }


    int check_bullet(Bullet &bullet) {
        if (!active || !bullet.active) return 0;
        SDL_FRect fr = rect();
        if (!SDL_HasRectIntersectionFloat(&bullet.rect, &fr)) return 0;

        bullet.active = false;
        hp--;

        if (hp <= 0) {
            active = false;
            return SCORE_FLEA;
        }

        speed = FLEA_SPEED_FAST;
        drop_index = (int)drop_ys.size();
        return 0;
    }

    bool update(float dt, float &out_x, float &out_y) {
        if (!active) return false;

        frame_timer -= dt;
        if (frame_timer <= 0.0f) {
            frame_timer = FLEA_FRAME_RATE;
            frame = (frame + 1) % FLEA_FRAMES;
        }

        y += speed * dt;

        bool should_drop = false;
        if (drop_index < (int)drop_ys.size() && y >= drop_ys[drop_index]) {
            out_x = x;
            out_y = drop_ys[drop_index];
            drop_index++;
            should_drop = true;
        }

        if (y > float(WINDOW_HEIGHT)) {
            active = false;
        }

        return should_drop;
    }

    void render(SDL_Renderer *renderer, SDL_Texture *sheet, SDL_Point palette) {
        if (!active) return;
        SDL_FRect src = src_rect(palette);
        SDL_FRect dst = rect();
        SDL_RenderTexture(renderer, sheet, &src, &dst);
    }
};