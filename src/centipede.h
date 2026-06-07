#pragma once

#include <vector>

#include <SDL3/SDL.h>

#include "constants.h"
#include "types.h"
#include "bullet.h"
#include "mushroom.h"
#include "wave_palette.h"

enum SegFacing { FACING_HORZ, FACING_DIAG, FACING_VERT };


inline SDL_FRect SEG_HEAD_HORIZ(SDL_Point o) { return { float(o.x +  4), float(o.y + 18), 8, 8 }; }
inline SDL_FRect SEG_HEAD_DIAG (SDL_Point o) { return { float(o.x +  4), float(o.y + 27), 8, 8 }; }
inline SDL_FRect SEG_HEAD_VERT (SDL_Point o) { return { float(o.x + 38), float(o.y + 27), 8, 8 }; }
inline SDL_FRect SEG_BODY_HORIZ(SDL_Point o) { return { float(o.x +  4), float(o.y + 36), 8, 8 }; }
inline SDL_FRect SEG_BODY_DIAG (SDL_Point o) { return { float(o.x +  4), float(o.y + 45), 8, 8 }; }
inline SDL_FRect SEG_BODY_VERT (SDL_Point o) { return { float(o.x + 38), float(o.y + 45), 8, 8 }; }


struct Segment {
    float x = 0.0f;
    float y = 0.0f;
    int hdir = -1;
    SegFacing facing = FACING_HORZ;

    SDL_FRect rect() const {
        return { x, y, float(CELL_PX), float(CELL_PX) };
    }
};


struct Waypoint {
    float x;
    float y;
    int hdir;
    SegFacing facing;
};


struct Centipede {
    std::vector<Segment> segments;
    std::vector<Waypoint> crumbs;
    float speed = CENTIPEDE_SPEED;
    bool head_poisoned = false;

    void init(int start_col, int start_row, int length, int direction) {
        segments.clear();
        crumbs.clear();
        s_target_y = float(start_row * CELL_PX);

        for (int i = 0; i < length; i++) {
            Segment s;
            s.x = float((start_col - i * direction) * CELL_PX);
            s.y = float(start_row * CELL_PX);
            s.hdir = direction;
            s.facing = FACING_HORZ;
            segments.push_back(s);
        }

        for (int i = length - 1; i >= 0; i--) {
            crumbs.push_back({ segments[i].x, segments[i].y, direction, FACING_HORZ });
        }
    }

    
    bool is_dead() const { return segments.empty(); }

    HitResult update(
        float dt,
        const std::vector<Mushroom> &mushrooms,
        Bullet &bullet,
        Centipede &out_split,
        bool &did_split)
    {
        HitResult result;
        did_split = false;

        result = check_bullet_collision(bullet, out_split, did_split);
        if (result.killed) return result;

        float dist = speed * dt;
        move_head(segments[0], dist, mushrooms);

        crumbs.insert(crumbs.begin(), { segments[0].x, segments[0].y, segments[0].hdir, segments[0].facing });

        for (int i = 1; i < (int)segments.size(); i++) {
            float target_dist = float(i * CELL_PX);
            float travelled   = 0.0f;

            for (int j = 0; j + 1 < (int)crumbs.size(); j++) {
                float dx   = crumbs[j].x - crumbs[j + 1].x;
                float dy   = crumbs[j].y - crumbs[j + 1].y;
                float step = SDL_sqrtf(dx * dx + dy * dy);

                if (travelled + step >= target_dist) {
                    float t = (target_dist - travelled) / step;
                    segments[i].x      = crumbs[j + 1].x + (crumbs[j].x - crumbs[j + 1].x) * (1.0f - t);
                    segments[i].y      = crumbs[j + 1].y + (crumbs[j].y - crumbs[j + 1].y) * (1.0f - t);
                    segments[i].hdir   = crumbs[j].hdir;
                    segments[i].facing = crumbs[j].facing;
                    break;
                }
                travelled += step;
            }
        }

        float max_dist = float(segments.size() * CELL_PX) + float(CELL_PX);
        float travelled = 0.0f;
        for (int j = 0; j + 1 < (int)crumbs.size(); j++) {
            float dx = crumbs[j].x - crumbs[j + 1].x;
            float dy = crumbs[j].y - crumbs[j + 1].y;
            travelled += SDL_sqrtf(dx * dx + dy * dy);
            if (travelled >= max_dist) {
                crumbs.resize(j + 2);
                break;
            }
        }

        return result;
    }

    void render(SDL_Renderer *renderer, SDL_Texture *sheet, SDL_Point palette) {
        for (int i = 0; i < (int)segments.size(); i++) {
            const Segment &s = segments[i];
            SDL_FRect dst    = s.rect();

            SDL_FRect src;
            if (i == 0) {
                if      (s.facing == FACING_HORZ) src = SEG_HEAD_HORIZ(palette);
                else if (s.facing == FACING_DIAG) src = SEG_HEAD_DIAG(palette);
                else                              src = SEG_HEAD_VERT(palette);
            } else {
                if      (s.facing == FACING_HORZ) src = SEG_BODY_HORIZ(palette);
                else if (s.facing == FACING_DIAG) src = SEG_BODY_DIAG(palette);
                else                              src = SEG_BODY_VERT(palette);
            }

            // flip horizontally when moving right
            SDL_FlipMode flip = (s.hdir == 1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

            SDL_RenderTextureRotated(renderer, sheet, &src, &dst, 0.0, nullptr, flip);
        }
    }


private:
    bool cell_blocked(int col, int row, const std::vector<Mushroom> &mushrooms) const {
        if (col < 0 || col >= GRID_COLS) return true;
        for (const auto &m : mushrooms) {
            if (!m.active) continue;
            if (int(m.rect.x) / CELL_PX == col &&
                int(m.rect.y) / CELL_PX == row) return true;
        }
        return false;
    }

    bool cell_poisoned(int col, int row, const std::vector<Mushroom> &mushrooms) const {
        for (const auto &m : mushrooms) {
            if (!m.active || !m.poisoned) continue;
            if (int(m.rect.x) / CELL_PX == col &&
                int(m.rect.y) / CELL_PX == row) return true;
        }
        return false;
    }

    void move_head(Segment &s, float dist, const std::vector<Mushroom> &mushrooms) {
        // ff poisoned, charge straight down ignoring walls and mushrooms
        if (head_poisoned) {
            s.y += dist;
            s.facing = FACING_VERT;
            if (s.y >= float(WINDOW_HEIGHT)) {
                s.y          = 0.0f;
                head_poisoned = false;
                s.facing     = FACING_HORZ;
            }
            return;
        }

        if (s.facing == FACING_DIAG || s.facing == FACING_VERT) {
            s.y += dist;
            if (s.facing == FACING_DIAG && s.y >= s_target_y - float(CELL_PX) * 0.5f)
                s.facing = FACING_VERT;
            if (s.y >= s_target_y) {
                s.y = s_target_y;
                s.facing = FACING_HORZ;
            }
        } else {
            float next_x = s.x + s.hdir * dist;

            if (s.hdir < 0 && next_x < 0.0f) {
                next_x = 0.0f;
                s.x = next_x;
                s.hdir = 1;
                s.facing = FACING_DIAG;
                s_target_y = s.y + float(CELL_PX);
                if (s_target_y >= float(WINDOW_HEIGHT)) s_target_y = 0.0f;
                s.y += dist;
                if (s.y >= s_target_y) { s.y = s_target_y; s.facing = FACING_HORZ; }
            } else if (s.hdir > 0 && next_x + float(CELL_PX) > float(WINDOW_WIDTH)) {
                next_x = float(WINDOW_WIDTH - CELL_PX);
                s.x = next_x;
                s.hdir = -1;
                s.facing = FACING_DIAG;
                s_target_y = s.y + float(CELL_PX);
                if (s_target_y >= float(WINDOW_HEIGHT)) s_target_y = 0.0f;
                s.y += dist;
                if (s.y >= s_target_y) { s.y = s_target_y; s.facing = FACING_HORZ; }
            } else {
                int next_col = (s.hdir > 0)
                    ? int(next_x + float(CELL_PX) - 1.0f) / CELL_PX
                    : int(next_x) / CELL_PX;

                if (cell_blocked(next_col, int(s.y) / CELL_PX, mushrooms)) {
                    if (cell_poisoned(next_col, int(s.y) / CELL_PX, mushrooms)) {
                        head_poisoned = true;
                        s.facing = FACING_VERT;
                    } else {
                        s.hdir = -s.hdir;
                        s.facing = FACING_DIAG;
                        s_target_y = s.y + float(CELL_PX);
                        if (s_target_y >= float(WINDOW_HEIGHT)) s_target_y = 0.0f;
                        s.y += dist;
                        if (s.y >= s_target_y) { s.y = s_target_y; s.facing = FACING_HORZ; }
                    }
                } else {
                    s.x = next_x;
                }
            }
        }
    }
    

    HitResult check_bullet_collision(Bullet &bullet, Centipede &out_split, bool &did_split) {
        HitResult result;
        if (!bullet.active) return result;

        for (int i = 0; i < (int)segments.size(); i++) {
            SDL_FRect r = segments[i].rect();
            if (!SDL_HasRectIntersectionFloat(&bullet.rect, &r)) continue;

            bullet.active = false;
            result.killed = true;
            result.killed_x = float(int(segments[i].x / CELL_PX) * CELL_PX);
            result.killed_y = float(int(segments[i].y / CELL_PX) * CELL_PX);

            if (i == 0) {
                segments.erase(segments.begin());
                crumbs.clear();
                if (!segments.empty()) {
                    crumbs.push_back({ segments[0].x, segments[0].y, segments[0].hdir, segments[0].facing });
                }
            } else {
                out_split.segments = std::vector<Segment>(segments.begin() + i + 1, segments.end());
                out_split.speed  = speed;
                out_split.crumbs = {};

                if (!out_split.segments.empty()) {
                    Segment &nh = out_split.segments[0];
                    nh.hdir = -nh.hdir;
                    nh.facing = FACING_DIAG;
                    out_split.s_target_y = nh.y + float(CELL_PX);

                    for (auto &seg : out_split.segments)
                        out_split.crumbs.push_back({ seg.x, seg.y, seg.hdir, seg.facing });
                }
                segments.resize(i);
                did_split = true;
            }
            return result;
        }
        return result;
    }

    float s_target_y = 0.0f;
    
};
