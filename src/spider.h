#pragma once

#include <algorithm>
#include <vector>

#include <SDL3/SDL.h>

#include "constants.h"
#include "mushroom.h"
#include "wave_palette.h"


enum SpiderState { SPIDER_ZIGZAG, SPIDER_VERTICAL, SPIDER_EXITING };


struct Spider {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    bool active = false;
    int frame = 0;
    float frame_timer = 0.0f;
    SpiderState mode = SPIDER_ZIGZAG;

    int wall_bounces_left = 0;
    float vertical_timer = 0.0f;

    float bound_top = 0.0f;
    float bound_bottom = 0.0f;

    SDL_FRect rect() const {
        return { x, y, float(SPIDER_W), float(SPIDER_H) };
    }

    SDL_FRect src_rect(SDL_Point palette) const {
        return {
            float(palette.x + frame * 17),
            float(palette.y + 54),
            float(SPIDER_FRAME_W),
            float(SPIDER_FRAME_H)
        };
    }

    void spawn() {
        active = true;
        frame = 0;
        frame_timer = 0.0f;
        mode = SPIDER_ZIGZAG;

        bound_top = PLAYER_ZONE_TOP - float(4 * CELL_PX);
        bound_bottom = float(WINDOW_HEIGHT - SPIDER_H);

        bool from_left = rand() % 2 == 0;
        vx = from_left ?  SPIDER_SPEED : -SPIDER_SPEED;
        vy = (rand() % 2 == 0) ? SPIDER_SPEED : -SPIDER_SPEED;

        x = from_left ? -float(SPIDER_W) : float(WINDOW_WIDTH);
        y = PLAYER_ZONE_TOP + float(rand() % (int)(bound_bottom - PLAYER_ZONE_TOP));

        wall_bounces_left = 2 + rand() % 4;
    }

    bool update(float dt, std::vector<Mushroom> &mushrooms) {
        if (!active) return false;

        // animate
        frame_timer -= dt;
        if (frame_timer <= 0.0f) {
            frame_timer = SPIDER_FRAME_RATE;
            frame = (frame + 1) % SPIDER_FRAMES;
        }

        if (mode == SPIDER_ZIGZAG) {
            x += vx * dt;
            y += vy * dt;

            // bounce top/bottom
            if (y < bound_top) {
                y  = bound_top;
                vy = SPIDER_SPEED;
                on_bounce();
            }
            if (y > bound_bottom) {
                y  = bound_bottom;
                vy = -SPIDER_SPEED;
                on_bounce();
            }

            // bounce side walls
            if (x < 0.0f) {
                x = 0.0f;
                vx = SPIDER_SPEED;
                wall_bounces_left--;
                on_bounce();
                if (wall_bounces_left <= 0) start_exiting(1);  // exit right
            }
            if (x + float(SPIDER_W) > float(WINDOW_WIDTH)) {
                x = float(WINDOW_WIDTH - SPIDER_W);
                vx = -SPIDER_SPEED;
                wall_bounces_left--;
                on_bounce();
                if (wall_bounces_left <= 0) start_exiting(-1);  // exit left
            }

        } else if (mode == SPIDER_VERTICAL) {
            y += vy * dt;

            // bounce top/bottom
            if (y < bound_top)    { y = bound_top;     vy =  SPIDER_SPEED; }
            if (y > bound_bottom) { y = bound_bottom;  vy = -SPIDER_SPEED; }

            vertical_timer -= dt;
            if (vertical_timer <= 0.0f) {
                // return to zigzag
                mode = SPIDER_ZIGZAG;
                vx = (rand() % 2 == 0) ? SPIDER_SPEED : -SPIDER_SPEED;
            }

        } else if (mode == SPIDER_EXITING) {
            x += vx * dt;
            y += vy * dt;

            // still bounce top/bottom while exiting
            if (y < bound_top)    { y = bound_top;     vy =  SPIDER_SPEED; }
            if (y > bound_bottom) { y = bound_bottom;  vy = -SPIDER_SPEED; }

            // off screen
            if (x + float(SPIDER_W) < 0.0f || x > float(WINDOW_WIDTH)) {
                active = false;
                return true;
            }
        }

        // eat mushrooms
        SDL_FRect sr = rect();
        for (auto &m : mushrooms) {
            if (!m.active) continue;
            if (SDL_HasRectIntersectionFloat(&sr, &m.rect)) {
                m.active = false;
            }
        }

        return false;
    }

    int check_bullet(Bullet &bullet, float player_cx) {
        if (!active || !bullet.active) return 0;
        SDL_FRect sr = rect();
        if (!SDL_HasRectIntersectionFloat(&bullet.rect, &sr)) return 0;

        bullet.active = false;
        active = false;

        float spider_cx = x + float(SPIDER_W) / 2.0f;
        float dist = SDL_fabsf(spider_cx - player_cx);

        if (dist < SPIDER_CLOSE_DIST) return SCORE_SPIDER_CLOSE;
        if (dist < SPIDER_MID_DIST)   return SCORE_SPIDER_MID;
        return SCORE_SPIDER_FAR;
    }

    void render(SDL_Renderer *renderer, SDL_Texture *sheet, SDL_Point palette) {
        if (!active) return;
        SDL_FRect src = src_rect(palette);
        SDL_FRect dst = rect();
        SDL_FlipMode flip = (vx >= 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderTextureRotated(renderer, sheet, &src, &dst, 0.0, nullptr, flip);
    }

private:
    void on_bounce() {
        if (mode == SPIDER_ZIGZAG && rand() % 3 == 0) {
            mode           = SPIDER_VERTICAL;
            vx             = 0.0f;
            vertical_timer = 0.5f + float(rand() % 16) / 10.0f;
        }
    }

    void start_exiting(int direction) {
        mode = SPIDER_EXITING;
        vx   = float(direction) * SPIDER_SPEED;
    }
};
