#pragma once

#include <vector>

#include <SDL3/SDL.h>

#include "constants.h"
#include "types.h"
#include "bullet.h"
#include "mushroom.h"
#include "wave_palette.h"

enum SegFacing { FACING_HORZ, FACING_DIAG, FACING_VERT };

static constexpr int SEG_FRAME_STRIDE  = 17;
static constexpr int SEG_FRAME_W       = 16;
static constexpr int SEG_FRAME_H       = 8;
static constexpr int SEG_HORZ_FRAMES   = 8;
static constexpr int SEG_DIAG_FRAMES   = 2;
static constexpr int SEG_VERT_FRAMES   = 4;
static constexpr float SEG_FRAME_RATE  = 0.04f;

inline SDL_FRect SEG_HEAD_HORIZ(SDL_Point o, int f) { return { float(o.x + f * SEG_FRAME_STRIDE),      float(o.y + 18), SEG_FRAME_W, SEG_FRAME_H }; }
inline SDL_FRect SEG_HEAD_DIAG (SDL_Point o, int f) { return { float(o.x + f * SEG_FRAME_STRIDE),      float(o.y + 27), SEG_FRAME_W, SEG_FRAME_H }; }
inline SDL_FRect SEG_HEAD_VERT (SDL_Point o, int f) { return { float(o.x + 34 + f * SEG_FRAME_STRIDE), float(o.y + 27), SEG_FRAME_W, SEG_FRAME_H }; }
inline SDL_FRect SEG_BODY_HORIZ(SDL_Point o, int f) { return { float(o.x + f * SEG_FRAME_STRIDE),      float(o.y + 36), SEG_FRAME_W, SEG_FRAME_H }; }
inline SDL_FRect SEG_BODY_DIAG (SDL_Point o, int f) { return { float(o.x + f * SEG_FRAME_STRIDE),      float(o.y + 45), SEG_FRAME_W, SEG_FRAME_H }; }
inline SDL_FRect SEG_BODY_VERT (SDL_Point o, int f) { return { float(o.x + 34 + f * SEG_FRAME_STRIDE), float(o.y + 45), SEG_FRAME_W, SEG_FRAME_H }; }


struct Segment {
    float x = 0.0f;
    float y = 0.0f;
    int hdir = -1;
    SegFacing facing = FACING_HORZ;

    SDL_FRect rect() const {
        return { x, y, float(CELL_PX), float(CELL_PX) };
    }

    SDL_FRect draw_rect() const {
        return { x - float(CELL_PX / 2), y, float(CELL_PX * 2), float(CELL_PX) };
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

    int frame       = 0;
    float frame_timer = 0.0f;

    int vdir = 1;

    bool head_poisoned  = false;
    int poison_hdir    = 1;
    bool poison_horiz   = false;
    float poison_hx_target = 0.0f;

    bool entering = false;

    void init(int start_col, int start_row, int length, int direction) {
        segments.clear();
        crumbs.clear();
        head_poisoned = false;
        poison_hdir = (rand() % 2 == 0) ? 1 : -1;
        poison_horiz = false;
        vdir = 1;
        entering = false;
        frame = 0;
        frame_timer = 0.0f;
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

    void init_entering(int length, float speed_override) {
        segments.clear();
        crumbs.clear();
        head_poisoned = false;
        poison_hdir = (rand() % 2 == 0) ? 1 : -1;
        poison_horiz = false;
        vdir = 1;
        entering = true;
        frame = 0;
        frame_timer = 0.0f;

        int center_col = GRID_COLS / 2;
        float cx = float(center_col * CELL_PX);

        for (int i = 0; i < length; i++) {
            Segment s;
            s.x = cx;
            s.y = float(-(i + 1) * CELL_PX);
            s.hdir = -1;
            s.facing = FACING_VERT;
            segments.push_back(s);
        }

        speed = speed_override;
        s_target_y = 0.0f;

        for (int i = (int)segments.size() - 1; i >= 0; i--) {
            crumbs.push_back({ segments[i].x, segments[i].y, -1, FACING_VERT });
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

        frame_timer -= dt;
        if (frame_timer <= 0.0f) {
            frame_timer = SEG_FRAME_RATE;
            frame = (frame + 1) % SEG_HORZ_FRAMES;
        }

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
            SDL_FRect dst    = s.draw_rect();

            int f_horz = frame % SEG_HORZ_FRAMES;
            int f_diag = frame % SEG_DIAG_FRAMES;
            int f_vert = frame % SEG_VERT_FRAMES;

            SDL_FRect src;
            if (i == 0) {
                if      (s.facing == FACING_HORZ) src = SEG_HEAD_HORIZ(palette, f_horz);
                else if (s.facing == FACING_DIAG) src = SEG_HEAD_DIAG(palette,  f_diag);
                else                              src = SEG_HEAD_VERT(palette,  f_vert);
            } else {
                if      (s.facing == FACING_HORZ) src = SEG_BODY_HORIZ(palette, f_horz);
                else if (s.facing == FACING_DIAG) src = SEG_BODY_DIAG(palette,  f_diag);
                else                              src = SEG_BODY_VERT(palette,  f_vert);
            }

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

        if (entering) {
            s.y += dist;
            s.facing = FACING_VERT;
            if (s.y >= 0.0f) {
                s.y = 0.0f;
                entering = false;
                s.hdir = (rand() % 2 == 0) ? 1 : -1;
                s.facing = FACING_HORZ;
                s_target_y = 0.0f;
            }
            return;
        }

        if (head_poisoned) {
            if (poison_horiz) {
                float target_x = poison_hx_target;
                float step = poison_hdir * dist;
                s.x += step;
                s.facing = FACING_DIAG;

                bool arrived = (poison_hdir > 0) ? (s.x >= target_x) : (s.x <= target_x);
                if (arrived) {
                    s.x = target_x;
                    poison_horiz = false;
                    s.facing = FACING_VERT;
                    s_target_y = s.y + float(CELL_PX);
                    if (s_target_y >= float(WINDOW_HEIGHT)) {
                        head_poisoned = false;
                        s.facing = FACING_HORZ;
                    }
                }
            } else {
                s.y += dist;
                s.facing = FACING_VERT;
                if (s.y >= s_target_y) {
                    s.y = s_target_y;
                    if (s.y + float(CELL_PX) >= float(WINDOW_HEIGHT)) {
                        head_poisoned = false;
                        s.facing = FACING_HORZ;
                        return;
                    }
                    poison_horiz = true;
                    poison_hx_target = SDL_clamp(
                        s.x + float(poison_hdir * CELL_PX),
                        0.0f,
                        float((GRID_COLS - 1) * CELL_PX));
                    poison_hdir = -poison_hdir;
                }
            }
            return;
        }

        if (s.facing == FACING_DIAG || s.facing == FACING_VERT) {
            float step_y = float(vdir) * dist;
            s.y += step_y;
            if (s.facing == FACING_DIAG) {
                float mid = (vdir > 0) ? s_target_y - float(CELL_PX) * 0.5f : s_target_y + float(CELL_PX) * 0.5f;
                bool past_mid = (vdir > 0) ? (s.y >= mid) : (s.y <= mid);
                if (past_mid) s.facing = FACING_VERT;
            }
            bool arrived = (vdir > 0) ? (s.y >= s_target_y) : (s.y <= s_target_y);
            if (arrived) {
                s.y = s_target_y;
                s.facing = FACING_HORZ;
            }
            return;
        }

        float next_x = s.x + s.hdir * dist;
        bool hit_left  = (s.hdir < 0 && next_x < 0.0f);
        bool hit_right = (s.hdir > 0 && next_x + float(CELL_PX) > float(WINDOW_WIDTH));

        if (hit_left || hit_right) {
            s.x = hit_left ? 0.0f : float(WINDOW_WIDTH - CELL_PX);
            s.hdir = -s.hdir;
            s.facing = FACING_DIAG;
            step_into_next_row(s, dist);
            return;
        }
        int next_col = (s.hdir > 0)
            ? int(next_x + float(CELL_PX) - 1.0f) / CELL_PX
            : int(next_x) / CELL_PX;

        if (cell_blocked(next_col, int(s.y) / CELL_PX, mushrooms)) {
            if (cell_poisoned(next_col, int(s.y) / CELL_PX, mushrooms)) {
                head_poisoned = true;
                poison_horiz = false;
                s_target_y = s.y + float(CELL_PX);
                s.facing = FACING_VERT;
            } else {
                s.hdir = -s.hdir;
                s.facing = FACING_DIAG;
                step_into_next_row(s, dist);
            }
        } else {
            s.x = next_x;
        }
    }

    void step_into_next_row(Segment &s, float dist) {
        s_target_y = s.y + float(vdir * CELL_PX);

        if (s_target_y >= float(WINDOW_HEIGHT)) {
            vdir = -1;
            s_target_y = s.y - float(CELL_PX);
        }
        if (s_target_y < 0.0f) {
            vdir = 1;
            s_target_y = s.y + float(CELL_PX);
        }

        s.y += float(vdir) * dist;
        bool arrived = (vdir > 0) ? (s.y >= s_target_y) : (s.y <= s_target_y);
        if (arrived) { s.y = s_target_y; s.facing = FACING_HORZ; }
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
                out_split.speed = speed;
                out_split.vdir = vdir;
                out_split.crumbs = {};

                if (!out_split.segments.empty()) {
                    Segment &nh = out_split.segments[0];
                    nh.hdir = -nh.hdir;
                    nh.facing = FACING_DIAG;
                    out_split.s_target_y = nh.y + float(vdir * CELL_PX);

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
