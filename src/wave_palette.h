#pragma once

#include <SDL3/SDL.h>

static constexpr SDL_Point WAVE_ORIGINS[] = {
    {   0,   0 },
    { 170,   0 },
    { 340,   0 },
    { 510,   0 },
    {   0, 117 },
    { 170, 117 },
    { 340, 117 },
    { 510, 117 },
    {   0, 234 },
    { 170, 234 },
    { 340, 234 },
    { 510, 234 },
    { 170, 351 },
    { 340, 351 },
};


static constexpr int WAVE_PALETTE_COUNT = 14;
 
inline SDL_Point get_wave_origin(int wave) {
    int idx = (wave - 1) % WAVE_PALETTE_COUNT;
    return WAVE_ORIGINS[idx];
}
 