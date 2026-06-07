#pragma once

#include <cstdio>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "constants.h"
#include "bullet.h"
#include "player.h"
#include "font.h"
#include "mushroom.h"
#include "flea.h"
#include "spider.h"
#include "centipede.h"
#include "scorpion.h"
#include "audio.h"
#include "types.h"



class Game {
    public:
        Game();
        ~Game();

        bool init();
        void run();

    private:

        // SDL handles
        SDL_Window   *window   = nullptr;
        SDL_Renderer *renderer = nullptr;
        SDL_Texture  *sheet    = nullptr;
        bool          running  = false;
        
        // game objects
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

        // game state
        GameState state = SPAWNING_MUSHROOMS;
        int lives = PLAYER_LIVES;
        int wave = 1;
        int score = 0;
        float death_timer = 0.0f;

        // private methods
        void spawn_mushrooms(int count);
        void update_mushroom_spawns(float dt);
        void spawn_centipede();
        void check_player_death();
        void reset_after_death();
        void check_wave_complete();
        void start_next_heal();
        int count_lower_mushrooms();
        void poll_keyboard();
        void handle_events();
        void update(float dt);
        void render();
};