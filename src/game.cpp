#include "game.h"


Game::Game() {}

Game::~Game() {
    audio.free();
    if (sheet) { SDL_DestroyTexture(sheet); sheet = nullptr; }
    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (window) { SDL_DestroyWindow(window); window = nullptr; }
    SDL_Quit();
}

bool Game::init() {
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

void Game::run() {

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

/*
    PRIVATE METHODS

*/


void Game::spawn_mushrooms(int count) {
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


void Game::update_mushroom_spawns(float dt) {
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


void Game::spawn_centipede() {
    float spd = CENTIPEDE_SPEED + (wave - 1) * WAVE_SPEED_INC;
    spd = SDL_min(spd, CENTIPEDE_SPEED_MIN);
    Centipede c;
    c.init(GRID_COLS - 1, 0, CENTIPEDE_LENGTH, -1);
    c.speed = spd;
    centipedes.push_back(c);
}


void Game::check_player_death() {
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


void Game::reset_after_death() {
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


void Game::check_wave_complete() {
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

void Game::start_next_heal() {
    if (heal_queue.empty()) return;
    int idx = heal_queue.front();
    heal_queue.erase(heal_queue.begin());
    Mushroom &m = mushrooms[idx];
    heal_anim.start(m.rect.x, m.rect.y, idx);
}

int Game::count_lower_mushrooms() {
    int count = 0;
    float midpoint = float(WINDOW_HEIGHT) / 2.0f;
    for (const auto &m : mushrooms) {
        if (!m.active) continue;
        if (m.rect.y >= midpoint) count++;
    }
    return count;
}

void Game::poll_keyboard() {
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


void Game::handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) { running = false; }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) running = false;
    }
    poll_keyboard();
}


void Game::update(float dt) {
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


void Game::render() {
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