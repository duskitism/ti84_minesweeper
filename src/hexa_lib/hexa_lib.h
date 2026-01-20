// Glorified "library". Wraps the ti84 toolchain headers and includes utilities that I use when doing calculator things.  
#pragma once

#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <ti/sprintf.h>
#include <sys/timers.h>

#include <time.h>

#define SCREEN_WIDTH GFX_LCD_WIDTH
#define SCREEN_HEIGHT GFX_LCD_HEIGHT

typedef struct {
    int x;
    int y;
} IVec2;

bool ivec2_eq(IVec2 v1, IVec2 v2);
IVec2 ivec2_add(IVec2 a, IVec2 b);

IVec2 ivec2_new(int x, int y);

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Rectangle;

Rectangle rect_new(int x, int y, int width, int height);
Rectangle rect_new_v(IVec2 pos, IVec2 dims);

bool rect_collision_rect(Rectangle r1, Rectangle r2);

void rect_draw_565(Rectangle rect, uint8_t color);

typedef struct {
    clock_t duration;
    clock_t then;
} Cooldown;

Cooldown cooldown_new(uint64_t millis);
bool cooldown_done(Cooldown* c);