#pragma once

struct HitResult {
    bool killed = false;
    float killed_x = 0.0f;
    float killed_y = 0.0f;
};

enum GameState { PLAYING, DEAD, GAME_OVER, SPAWNING_MUSHROOMS, HEALING_MUSHROOMS, MAIN_MENU };