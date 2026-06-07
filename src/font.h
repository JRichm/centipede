#pragma once

#include <cstdio>

#include <SDL3/SDL.h>
#include "wave_palette.h"


struct Font {
    SDL_Texture *sheet = nullptr;

    SDL_FRect get_char_rect(char c, int ox, int oy) const {
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
            float(ox + col * 9),
            float(oy + 90 + row * 9),
            8.0f,
            8.0f
        };
    }

    void draw_string(
        SDL_Renderer *renderer,
        const char *text,
        float x,
        float y,
        float scale,
        SDL_Point palette = {0, 0}
    ) const {
        float cursor_x = x;
        for (int i = 0; text[i] != '\0'; i++) {
            char c = text[i];
            if (c == ' ') {
                cursor_x += 8.0f * scale + scale;
                continue;
            }

            if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';

            SDL_FRect src = get_char_rect(c, palette.x, palette.y);
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

    void draw_int(
        SDL_Renderer *renderer,
        int value,
        float x,
        float y,
        float scale,
        SDL_Point palette = {0, 0}
    ) const {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", value);
        draw_string(renderer, buf, x, y, scale, palette);
    }
};
