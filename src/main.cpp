#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>

#define WINDOW_TITLE  "Centipede"
#define CELL_SIZE 8
#define GRID_COLS 30
#define GRID_ROWS 32

#define WINDOW_SCALE 4
#define WINDOW_WIDTH (GRID_COLS * CELL_SIZE * WINDOW_SCALE)
#define WINDOW_HEIGHT (GRID_ROWS * CELL_SIZE * WINDOW_SCALE)

#define CELL_PX (CELL_SIZE * WINDOW_SCALE) // pixel size of one cell

#define PLAYER_ZONE_TOP (WINDOW_HEIGHT * 0.70f)

#define SPRITE_W 7
#define SPRITE_H 8
#define PLAYER_W (SPRITE_W * WINDOW_SCALE)
#define PLAYER_H (SPRITE_H * WINDOW_SCALE)
#define PLAYER_SPEED (100.0f * float(WINDOW_SCALE))

#define BULLET_W (1 * WINDOW_SCALE)
#define BULLET_H (6 * WINDOW_SCALE)
#define BULLET_SPEED (400.0f * float(WINDOW_SCALE))

#define MUSHROOM_SPRITE_W 8
#define MUSHROOM_SPRITE_H 8
#define MUSHROOM_W (MUSHROOM_SPRITE_W * WINDOW_SCALE)
#define MUSHROOM_H (MUSHROOM_SPRITE_H * WINDOW_SCALE)
#define MUSHROOM_MAX_HP 4

#define CENTIPEDE_SPEED (60.0f * float(WINDOW_SCALE))
#define CENTIPEDE_SPEED_MIN (180.0f * float(WINDOW_SCALE))
#define CENTIPEDE_SPEED_INC (10.0f * float(WINDOW_SCALE))
#define CENTIPEDE_LENGTH 12

#define SPIDER_FRAME_W 16
#define SPIDER_FRAME_H 8
#define SPIDER_W (SPIDER_FRAME_W * WINDOW_SCALE)
#define SPIDER_H (SPIDER_FRAME_H * WINDOW_SCALE)
#define SPIDER_SPEED (80.0f * float(WINDOW_SCALE))
#define SPIDER_FRAME_RATE 0.08f
#define SPIDER_SPAWN_RATE 15.0f
#define SPIDER_FRAMES 8
#define SPIDER_CLOSE_DIST (3 * CELL_PX)
#define SPIDER_MID_DIST (6 * CELL_PX)

#define FLEA_FRAME_W 16
#define FLEA_FRAME_H 8
#define FLEA_W (FLEA_FRAME_W * WINDOW_SCALE)
#define FLEA_H (FLEA_FRAME_H * WINDOW_SCALE)
#define FLEA_SPEED (185.0f * float(WINDOW_SCALE))
#define FLEA_SPEED_FAST (250.0f * float(WINDOW_SCALE))
#define FLEA_FRAME_RATE 0.08f
#define FLEA_FRAMES 4
#define FLEA_MUSHROOM_MIN 5
#define FLEA_MUSHROOM_MAX 8
#define FLEA_SPAWN_THRESHOLD 5

#define SCORPION_FRAME_W 16
#define SCORPION_FRAME_H 8
#define SCORPION_W (SCORPION_FRAME_W * WINDOW_SCALE)
#define SCORPION_H (SCORPION_FRAME_H * WINDOW_SCALE)
#define SCORPION_SPEED (70.0f * float(WINDOW_SCALE))
#define SCORPION_FRAME_RATE 0.08f
#define SCORPION_FRAMES 4
#define SCORPION_SPAWN_RATE 20.0f
#define SCORPION_WAVE_MIN 3

#define PLAYER_LIVES 3
#define DEATH_DELAY 1.5f
#define WAVE_CLEAR_DELAY 1.0f
#define MUSHROOM_SPAWN_INTERVAL 0.05f

#define SCORE_MUSHROOM 1
#define SCORE_CENTIPEDE_BODY 10
#define SCORE_CENTIPEDE_HEAD 100
#define SCORE_SPIDER_CLOSE 900
#define SCORE_SPIDER_MID 600
#define SCORE_SPIDER_FAR 300
#define SCORE_FLEA 200
#define SCORE_SCORPION 1000

#define HEAL_FRAMES 6
#define HEAL_FRAME_RATE 0.08f

#define WAVE_SPEED_INC 10.0f

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

static constexpr SDL_FRect SEG_HEAD_HORIZ   = {  4, 18, 8, 8 };
static constexpr SDL_FRect SEG_HEAD_DIAG    = {  4, 27, 8, 8 };
static constexpr SDL_FRect SEG_HEAD_VERT    = { 38, 27, 8, 8 };
static constexpr SDL_FRect SEG_BODY_HORIZ   = {  4, 36, 8, 8 };
static constexpr SDL_FRect SEG_BODY_DIAG    = {  4, 45, 8, 8 };
static constexpr SDL_FRect SEG_BODY_VERT    = { 38, 45, 8, 8 };



enum GameState { PLAYING, DEAD, GAME_OVER, SPAWNING_MUSHROOMS, HEALING_MUSHROOMS };
enum SegFacing { FACING_HORZ, FACING_DIAG, FACING_VERT };
enum SpiderState { SPIDER_ZIGZAG, SPIDER_VERTICAL, SPIDER_EXITING };


struct InputState {
    float dx = 0.0f;
    float dy = 0.0f;
    bool fire = false;
};


struct HitResult {
    bool killed = false;
    float killed_x   = 0.0f;
    float killed_y   = 0.0f;
};


struct Font {
    SDL_Texture *sheet = nullptr;

    SDL_FRect get_char_rect(char c) const {
        int col = 0;
        int row = 0;

        if (c >= 'A' && c <= 'O') {
            col = c - 'A';
            row = 0;
        } else if (c >= 'P' && c <= 'Z') {
            col = c - 'P';
            row = 1;
        } else if (c >= '0' && c <= '9') {
            col = c - '0';
            row = 2;
        } else {
            return { 0, 0, 0, 0 };
        }

        return {
            float(col * 9),
            float(90 + row * 9),
            8.0f,
            8.0f
        };
    }

    void draw_string(SDL_Renderer *renderer, const char *text, float x, float y, float scale) const {
        float cursor_x = x;
        for (int i = 0; text[i] != '\0'; i++) {
            char c = text[i];
            if (c == ' ') {
                cursor_x += 8.0f * scale + scale;
                continue;
            }

            if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';

            SDL_FRect src = get_char_rect(c);
            if (src.w == 0) {
                cursor_x += 8.0f * scale + scale;
                continue;
            }

            SDL_FRect dst = {
                cursor_x,
                y,
                8.0f * scale,
                8.0f * scale
            };

            SDL_RenderTexture(renderer, sheet, &src, &dst);
            cursor_x += 8.0f * scale + scale;
        }
    }

    void draw_int(SDL_Renderer *renderer, int value,
                  float x, float y, float scale) const {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", value);
        draw_string(renderer, buf, x, y, scale);
    }
};


struct Audio {

    MIX_Mixer *mixer = nullptr;

    // loaded sounds
    MIX_Audio *shoot     = nullptr;
    MIX_Audio *kill      = nullptr;
    MIX_Audio *death     = nullptr;
    MIX_Audio *bonus     = nullptr;
    MIX_Audio *centipede = nullptr;
    MIX_Audio *spider    = nullptr;
    MIX_Audio *flea      = nullptr;
    MIX_Audio *scorpion  = nullptr;

    // all sounds on dedicated tracks for gain control
    MIX_Track *track_shoot      = nullptr;
    MIX_Track *track_kill       = nullptr;
    MIX_Track *track_death      = nullptr;
    MIX_Track *track_bonus      = nullptr;
    MIX_Track *track_centipede  = nullptr;
    MIX_Track *track_spider     = nullptr;
    MIX_Track *track_flea       = nullptr;
    MIX_Track *track_scorpion   = nullptr;

    // tune these to taste (1.0 = normal, 0.5 = half, 2.0 = double)
    static constexpr float GAIN_SHOOT     = 0.5f;
    static constexpr float GAIN_KILL      = 0.8f;
    static constexpr float GAIN_DEATH     = 1.0f;
    static constexpr float GAIN_BONUS     = 0.9f;
    static constexpr float GAIN_CENTIPEDE = 0.2f;
    static constexpr float GAIN_SPIDER    = 1.2f;
    static constexpr float GAIN_FLEA      = 0.8f;
    static constexpr float GAIN_SCORPION  = 0.1f;

    bool init() {
        if (!MIX_Init()) {
            fprintf(stderr, "MIX_Init error: %s\n", SDL_GetError());
            return false;
        }

        mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!mixer) {
            fprintf(stderr, "MIX_CreateMixerDevice error: %s\n", SDL_GetError());
            return false;
        }

        // load all sounds
        shoot     = MIX_LoadAudio(mixer, "assets/sounds/shoot.wav",     true);
        kill      = MIX_LoadAudio(mixer, "assets/sounds/kill.wav",      true);
        death     = MIX_LoadAudio(mixer, "assets/sounds/death.wav",     true);
        bonus     = MIX_LoadAudio(mixer, "assets/sounds/bonus.wav",     true);
        centipede = MIX_LoadAudio(mixer, "assets/sounds/centipede.wav", true);
        spider    = MIX_LoadAudio(mixer, "assets/sounds/spider.wav",    true);
        flea      = MIX_LoadAudio(mixer, "assets/sounds/flea.wav",      true);
        scorpion  = MIX_LoadAudio(mixer, "assets/sounds/scorpion.wav",  true);

        if (!shoot || !kill || !death || !bonus ||
            !centipede || !spider || !flea || !scorpion) {
            fprintf(stderr, "Error loading sounds: %s\n", SDL_GetError());
            return false;
        }

        // create all tracks
        track_shoot     = MIX_CreateTrack(mixer);
        track_kill      = MIX_CreateTrack(mixer);
        track_death     = MIX_CreateTrack(mixer);
        track_bonus     = MIX_CreateTrack(mixer);
        track_centipede = MIX_CreateTrack(mixer);
        track_spider    = MIX_CreateTrack(mixer);
        track_flea      = MIX_CreateTrack(mixer);
        track_scorpion  = MIX_CreateTrack(mixer);

        if (!track_shoot || !track_kill || !track_death || !track_bonus ||
            !track_centipede || !track_spider || !track_flea || !track_scorpion) {
            fprintf(stderr, "Error creating tracks: %s\n", SDL_GetError());
            return false;
        }

        // assign audio to tracks
        MIX_SetTrackAudio(track_shoot,     shoot);
        MIX_SetTrackAudio(track_kill,      kill);
        MIX_SetTrackAudio(track_death,     death);
        MIX_SetTrackAudio(track_bonus,     bonus);
        MIX_SetTrackAudio(track_centipede, centipede);
        MIX_SetTrackAudio(track_spider,    spider);
        MIX_SetTrackAudio(track_flea,      flea);
        MIX_SetTrackAudio(track_scorpion,  scorpion);

        // set looping tracks to loop forever
        MIX_SetTrackLoops(track_centipede, -1);
        MIX_SetTrackLoops(track_spider,    -1);
        MIX_SetTrackLoops(track_flea,      -1);
        MIX_SetTrackLoops(track_scorpion,  -1);

        // set gain for each track
        MIX_SetTrackGain(track_shoot,     GAIN_SHOOT);
        MIX_SetTrackGain(track_kill,      GAIN_KILL);
        MIX_SetTrackGain(track_death,     GAIN_DEATH);
        MIX_SetTrackGain(track_bonus,     GAIN_BONUS);
        MIX_SetTrackGain(track_centipede, GAIN_CENTIPEDE);
        MIX_SetTrackGain(track_spider,    GAIN_SPIDER);
        MIX_SetTrackGain(track_flea,      GAIN_FLEA);
        MIX_SetTrackGain(track_scorpion,  GAIN_SCORPION);

        return true;
    }

    void free() {
        MIX_StopAllTracks(mixer, 0);
        MIX_DestroyTrack(track_shoot);
        MIX_DestroyTrack(track_kill);
        MIX_DestroyTrack(track_death);
        MIX_DestroyTrack(track_bonus);
        MIX_DestroyTrack(track_centipede);
        MIX_DestroyTrack(track_spider);
        MIX_DestroyTrack(track_flea);
        MIX_DestroyTrack(track_scorpion);
        MIX_DestroyAudio(shoot);
        MIX_DestroyAudio(kill);
        MIX_DestroyAudio(death);
        MIX_DestroyAudio(bonus);
        MIX_DestroyAudio(centipede);
        MIX_DestroyAudio(spider);
        MIX_DestroyAudio(flea);
        MIX_DestroyAudio(scorpion);
        MIX_DestroyMixer(mixer);
        MIX_Quit();
    }

    // one-shot helpers — stop, rewind, play so rapid firing restarts cleanly
    void play_oneshot(MIX_Track *track) {
        if (!track) return;
        MIX_StopTrack(track, 0);
        MIX_SetTrackPlaybackPosition(track, 0);
        MIX_PlayTrack(track, 0);
    }

    void play_shoot() { play_oneshot(track_shoot); }
    void play_kill()  { play_oneshot(track_kill);  }
    void play_death() { play_oneshot(track_death); }
    void play_bonus() { play_oneshot(track_bonus); }

    void start_loop(MIX_Track *track) {
        if (track && !MIX_TrackPlaying(track))
            MIX_PlayTrack(track, 0);
    }
    void stop_loop(MIX_Track *track) {
        if (track && MIX_TrackPlaying(track))
            MIX_StopTrack(track, 0);
    }

    void update_centipede(bool playing) {
        playing ? start_loop(track_centipede) : stop_loop(track_centipede);
    }
    void update_spider(bool playing) {
        playing ? start_loop(track_spider) : stop_loop(track_spider);
    }
    void update_flea(bool playing) {
        playing ? start_loop(track_flea) : stop_loop(track_flea);
    }
    void update_scorpion(bool playing) {
        playing ? start_loop(track_scorpion) : stop_loop(track_scorpion);
    }
};


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

    void render(SDL_Renderer *renderer, SDL_Texture *sheet) {
        for (int i = 0; i < (int)segments.size(); i++) {
            const Segment &s = segments[i];
            SDL_FRect dst    = s.rect();

            const SDL_FRect *src;
            if (i == 0) {
                if      (s.facing == FACING_HORZ) src = &SEG_HEAD_HORIZ;
                else if (s.facing == FACING_DIAG) src = &SEG_HEAD_DIAG;
                else                              src = &SEG_HEAD_VERT;
            } else {
                if      (s.facing == FACING_HORZ) src = &SEG_BODY_HORIZ;
                else if (s.facing == FACING_DIAG) src = &SEG_BODY_DIAG;
                else                              src = &SEG_BODY_VERT;
            }

            // flip horizontally when moving right
            SDL_FlipMode flip = (s.hdir == 1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

            SDL_RenderTextureRotated(renderer, sheet, src, &dst, 0.0, nullptr, flip);
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

    SDL_FRect src_rect() const {
        return { float(frame * 17), 54.0f,
                 float(SPIDER_FRAME_W), float(SPIDER_FRAME_H) };
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

    void render(SDL_Renderer *renderer, SDL_Texture *sheet) {
        if (!active) return;
        SDL_FRect src = src_rect();
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

    SDL_FRect src_rect() const {
        return { float(frame * 17), 63.0f,
                 float(FLEA_FRAME_W), float(FLEA_FRAME_H) };
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

    void render(SDL_Renderer *renderer, SDL_Texture *sheet) {
        if (!active) return;
        SDL_FRect src = src_rect();
        SDL_FRect dst = rect();
        SDL_RenderTexture(renderer, sheet, &src, &dst);
    }
};


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

    SDL_FRect src_rect() const { return { float(frame * 17), 72.0f, float(SCORPION_FRAME_W), float(SCORPION_FRAME_H) }; }

    void spawn() {
        active = true;
        frame = 0;
        frame_timer = 0.0f;

        int max_row = int(PLAYER_ZONE_TOP / CELL_PX) - 1;
        int row = 1 + rand() % SDL_max(1, max_row);
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

    void render(SDL_Renderer *renderer, SDL_Texture *sheet) {
        if (!active) return;
        SDL_FRect src = src_rect();
        SDL_FRect dst = rect();

        SDL_FlipMode flip = (vx > 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderTextureRotated(renderer, sheet, &src, &dst, 0.0, nullptr, flip);
    }
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


class Game {
    public:
        Game() : window(nullptr), renderer(nullptr), running(false) {}

        ~Game() {
            audio.free();
            if (sheet) { SDL_DestroyTexture(sheet); sheet = nullptr; }
            if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
            if (window) { SDL_DestroyWindow(window); window = nullptr; }
            SDL_Quit();
        }

        bool init() {
            if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
                fprintf(stderr, "Error initializing SDL3: %s\n", SDL_GetError());
                return false;
            }

            if (!audio.init()) return false;

            window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
            if (!window) {
                fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
                return false;
            }

            renderer = SDL_CreateRenderer(window, nullptr);
            if (!renderer) {
                fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
                return false;
            }

            if (!player.init()) {
                return false;
            }

            sheet = IMG_LoadTexture(renderer, "assets/sheet_transparent.png");
            if (!sheet) {
                fprintf(stderr, "Error loading sprite sheet: %s\n", SDL_GetError());
                return false;
            }
            SDL_SetTextureScaleMode(sheet, SDL_SCALEMODE_NEAREST);
            font.sheet = sheet;

            spawn_mushrooms(40);
            state = SPAWNING_MUSHROOMS;

            running = true;
            return true;
        }

        void run() {

            Uint64 last = SDL_GetTicks();

            while (running) {
                Uint64 now = SDL_GetTicks();
                float dt = (now - last) / 1000.0f;
                last = now;

                handle_events();
                update(dt);
                render();
            }
        }

    private:
        SDL_Window   *window;
        SDL_Renderer *renderer;
        SDL_Texture  *sheet;
        bool running;
        Player player;
        Bullet bullet;
        InputState input;
        Font font;

        Audio audio;

        std::vector<Centipede> centipedes;

        Spider spider;
        float spider_spawn_timer = SPIDER_SPAWN_RATE;

        Flea  flea;
        float flea_check_timer = 3.0f;
        
        Scorpion scorpion;
        float scorpion_spawn_timer = SCORPION_SPAWN_RATE;

        std::vector<Mushroom>  mushrooms;
        std::vector<std::pair<float,float>> mushroom_spawn_queue;
        float mushroom_spawn_timer = 0.0f;

        HealAnimation heal_anim;
        std::vector<int> heal_queue;
        bool healing_done = false;

        GameState state = SPAWNING_MUSHROOMS;
        int lives = PLAYER_LIVES;
        int wave = 1;
        int score = 0;
        float death_timer = 0.0f;

        
        void spawn_mushrooms(int count) {
            for (int i = 0; i < count; i++) {
                int col = rand() % GRID_COLS;
                int row = rand() % (int)(GRID_ROWS * 0.75f);
                float x = float(col * CELL_PX);
                float y = float(row * CELL_PX);
                bool occupied = false;
                
                for (const auto &m : mushrooms) {
                    if (!m.active) continue;
                    if (m.rect.x == x && m.rect.y == y) { occupied = true; break; }
                }

                for (const auto &q : mushroom_spawn_queue) {
                    if (q.first == x && q.second == y) { occupied = true; break; }
                }

                if (!occupied)
                    mushroom_spawn_queue.push_back({ x, y });
            }
        }


        void update_mushroom_spawns(float dt) {
            if (mushroom_spawn_queue.empty()) return;

            mushroom_spawn_timer -= dt;
            if (mushroom_spawn_timer > 0.0f) return;
            mushroom_spawn_timer = MUSHROOM_SPAWN_INTERVAL;

            auto [x, y] = mushroom_spawn_queue.front();
            mushroom_spawn_queue.erase(mushroom_spawn_queue.begin());

            Mushroom m;
            m.init(x, y);
            mushrooms.push_back(m);
        }


        void spawn_centipede() {
            float spd = CENTIPEDE_SPEED + (wave - 1) * WAVE_SPEED_INC;
            spd = SDL_min(spd, CENTIPEDE_SPEED_MIN);
            Centipede c;
            c.init(GRID_COLS - 1, 0, CENTIPEDE_LENGTH, -1);
            c.speed = spd;
            centipedes.push_back(c);
        }


        void check_player_death() {
            for (const auto &c : centipedes) {
                for (const auto &s : c.segments) {
                    SDL_FRect sr = s.rect();
                    if (SDL_HasRectIntersectionFloat(&player.rect, &sr)) {
                        lives --;
                        state = (lives <= 0) ? GAME_OVER : DEAD;
                        death_timer = DEATH_DELAY;
                        audio.play_death();
                        return;
                    }
                }
            }
        }


        void reset_after_death() {
            // reset player position
            player.rect.x = (WINDOW_WIDTH - PLAYER_W) / 2.0f;
            player.rect.y = WINDOW_HEIGHT - PLAYER_H - 16.0f;

            // clear bullet
            bullet.active = false;

            //reset centipede
            centipedes.clear();
            spawn_centipede();

            state = PLAYING;

            audio.update_spider(false);
            audio.update_flea(false);
            audio.update_scorpion(false);
        }


        void check_wave_complete() {
            if (!centipedes.empty()) return;
            if (state == SPAWNING_MUSHROOMS || state == HEALING_MUSHROOMS) return;

            wave++;
            audio.play_bonus();

            heal_queue.clear();
            for (int i = 0; i < (int)mushrooms.size(); i++) {
                if (mushrooms[i].active && mushrooms[i].poisoned)
                    heal_queue.push_back(i);
            }

            if (!heal_queue.empty()) {
                start_next_heal();
                state = HEALING_MUSHROOMS;
            } else {
                spawn_centipede();
                state = PLAYING;
            }
        }

        void start_next_heal() {
            if (heal_queue.empty()) return;
            int idx = heal_queue.front();
            heal_queue.erase(heal_queue.begin());
            Mushroom &m = mushrooms[idx];
            heal_anim.start(m.rect.x, m.rect.y, idx);
        }

        int count_lower_mushrooms() {
            int count = 0;
            float midpoint = float(WINDOW_HEIGHT) / 2.0f;
            for (const auto &m : mushrooms) {
                if (!m.active) continue;
                if (m.rect.y >= midpoint) count++;
            }
            return count;
        }

        void poll_keyboard() {
            const bool *keys = SDL_GetKeyboardState(nullptr);
            input.dx = 0.0f;
            input.dy = 0.0f;
            input.fire = false;
            if (keys[SDL_SCANCODE_A]) input.dx -= 1.0f;
            if (keys[SDL_SCANCODE_D]) input.dx += 1.0f;
            if (keys[SDL_SCANCODE_W]) input.dy -= 1.0f;
            if (keys[SDL_SCANCODE_S]) input.dy += 1.0f;
            if (keys[SDL_SCANCODE_SPACE]) input.fire = true;
        }


        void handle_events() {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) { running = false; }
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) running = false;
            }
            poll_keyboard();
        }


        void update(float dt) {
            if (state == GAME_OVER) return;

            if (state == DEAD) {
                death_timer -= dt;
                if (death_timer <= 0.0f) reset_after_death();
                return;
            }

            if (state == SPAWNING_MUSHROOMS) {
                update_mushroom_spawns(dt);
                if (mushroom_spawn_queue.empty()) {
                    spawn_centipede();
                    state = PLAYING;
                }
                return;
            }

            if (state == HEALING_MUSHROOMS) {
                if (heal_anim.update(dt)) {
                    // Animation done — heal the mushroom
                    int idx = heal_anim.mushroom_index;
                    if (idx >= 0 && idx < (int)mushrooms.size()) {
                        mushrooms[idx].poisoned = false;
                        mushrooms[idx].hp       = MUSHROOM_MAX_HP;
                    }

                    if (!heal_queue.empty()) {
                        start_next_heal();

                    } else {
                        spawn_centipede();
                        state = PLAYING;
                    }
                }
                return;
            }

            bool bullet_was_active = bullet.active;
            player.update(input, dt, bullet);
            if (!bullet_was_active && bullet.active)
                audio.play_shoot();
            
            bullet.update(dt);
            update_mushroom_spawns(dt);

            for (auto &m : mushrooms) {
                bool was = m.active;
                m.check_bullet(bullet);
                if (was && !m.active) score += SCORE_MUSHROOM;
            }

            // spider
            spider_spawn_timer -= dt;
            if (spider_spawn_timer <= 0.0f && !spider.active) {
                spider.spawn();
                spider_spawn_timer = SPIDER_SPAWN_RATE;
            }

            spider.update(dt, mushrooms);

            int spider_points = spider.check_bullet(bullet, player.rect.x + player.rect.w / 2.0f);
            if (spider_points > 0)
                score += spider_points;

            if (spider.active) {
                SDL_FRect sr = spider.rect();
                if (SDL_HasRectIntersectionFloat(&player.rect, &sr)) {
                    spider.active = false;
                    lives--;
                    state       = (lives <= 0) ? GAME_OVER : DEAD;
                    death_timer = DEATH_DELAY;
                    audio.play_death();
                }
            }

            // flea
            // Flea spawn check
            flea_check_timer -= dt;
            if (flea_check_timer <= 0.0f) {
                flea_check_timer = 3.0f;
                if (!flea.active && count_lower_mushrooms() <= FLEA_SPAWN_THRESHOLD)
                    flea.spawn();
            }

            // Flea update
            float drop_x, drop_y;
            if (flea.update(dt, drop_x, drop_y)) {
                // Try to spawn a mushroom at the drop position
                bool occupied = false;
                for (const auto &m : mushrooms) {
                    if (!m.active) continue;
                    if (m.rect.x == drop_x && m.rect.y == drop_y) {
                        occupied = true; break;
                    }
                }
                if (!occupied) {
                    Mushroom m;
                    m.init(drop_x, drop_y);
                    mushrooms.push_back(m);
                }
            }

            // Flea bullet check
            int flea_points = flea.check_bullet(bullet);
            if (flea_points > 0) {
                score += flea_points;
                audio.play_kill();
            }

            // Flea touches player
            if (flea.active) {
                SDL_FRect fr = flea.rect();
                if (SDL_HasRectIntersectionFloat(&player.rect, &fr)) {
                    flea.active = false;
                    lives--;
                    state       = (lives <= 0) ? GAME_OVER : DEAD;
                    death_timer = DEATH_DELAY;
                    audio.play_death();
                }
            }

            if (wave >= SCORPION_WAVE_MIN) {
                scorpion_spawn_timer -= dt;
                if (scorpion_spawn_timer <= 0.0f && !scorpion.active) {
                    scorpion.spawn();
                    scorpion_spawn_timer = SCORPION_SPAWN_RATE;
                }
            }

            scorpion.update(dt, mushrooms);

            int scorpion_points = scorpion.check_bullet(bullet);
            if (scorpion_points > 0) {
                score += scorpion_points;
                audio.play_kill();
            }

            // centipedes
            std::vector<Centipede> new_centipedes;
            for (auto &c : centipedes) {
                Centipede split;
                bool did_split = false;
                HitResult hit = c.update(dt, mushrooms, bullet, split, did_split);

                if (hit.killed) {
                    score += SCORE_CENTIPEDE_BODY;
                    audio.play_kill();

                    float snapped_x = float(int(hit.killed_x / CELL_PX) * CELL_PX);
                    float snapped_y = float(int(hit.killed_y / CELL_PX) * CELL_PX);

                    bool occupied = false;
                    for (const auto &m : mushrooms) {
                        if (!m.active) continue;
                        if (m.rect.x == snapped_x && m.rect.y == snapped_y) {
                            occupied = true; break;
                        }
                    }
                    if (!occupied) {
                        Mushroom m;
                        m.init(snapped_x, snapped_y);
                        mushrooms.push_back(m);
                    }
                }

                if (did_split && !split.segments.empty())
                    new_centipedes.push_back(split);
            }

            for (auto &c : new_centipedes)
                centipedes.push_back(c);

            centipedes.erase(
                std::remove_if(centipedes.begin(), centipedes.end(),
                    [](const Centipede &c) { return c.is_dead(); }),
                centipedes.end());
                
            // Update looping sounds
            audio.update_centipede(!centipedes.empty());
            audio.update_spider(spider.active);
            audio.update_flea(flea.active);
            audio.update_scorpion(scorpion.active);

            check_wave_complete();
            check_player_death();
        }


        void render() {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            
            for (auto &m : mushrooms) m.render(renderer, sheet);
            heal_anim.render(renderer, sheet);
            for (auto &c : centipedes) c.render(renderer, sheet);

            spider.render(renderer, sheet);
            flea.render(renderer, sheet);
            scorpion.render(renderer, sheet);

            if (state != DEAD)
                player.render(renderer, sheet);

            bullet.render(renderer);

            font.draw_int(renderer, score, 8, 8, WINDOW_SCALE);

            font.draw_int(renderer, lives, WINDOW_WIDTH - 120, 8, WINDOW_SCALE);

            if (state == DEAD) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 80);
                SDL_FRect overlay = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
                SDL_RenderFillRect(renderer, &overlay);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }

            if (state == GAME_OVER) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                SDL_FRect overlay = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
                SDL_RenderFillRect(renderer, &overlay);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                font.draw_string(renderer, "GAME OVER", 
                    WINDOW_WIDTH / 2.0f - (9 * 9 * WINDOW_SCALE) / 2.0f,
                    WINDOW_HEIGHT / 2.0f - 4 * WINDOW_SCALE,
                    WINDOW_SCALE);
            }

            SDL_RenderPresent(renderer);
        }
};

int main() {
    Game game;

    if (!game.init()) {
        return EXIT_FAILURE;
    }

    game.run();
    return EXIT_SUCCESS;
}