#include "hexa_lib.h"

bool ivec2_eq(IVec2 v1, IVec2 v2) {
    return v1.x == v2.x && v1.y == v2.y;
}

IVec2 ivec2_new(int x, int y) {
    return (IVec2) { x, y };
}

IVec2 ivec2_add(IVec2 a, IVec2 b) {
    return (IVec2) { a.x + b.x, a.y + b.y };
}

Rectangle rect_new(int x, int y, int width, int height) {
    return (Rectangle) { x, y, width, height }; 
}

Rectangle rect_new_v(IVec2 pos, IVec2 dims) {
    return (Rectangle) { pos.x, pos.y, dims.x, dims.y };  
}

bool rect_collision_rect(Rectangle r1, Rectangle r2) {
    return gfx_CheckRectangleHotspot(
        r1.x, r1.y, r1.width, r1.height, 
        r2.x, r2.y, r2.width, r2.height
    );
}

void rect_draw_565(Rectangle rect, uint8_t color) {
    gfx_SetColor(color);
    gfx_FillRectangle(rect.x, rect.y, rect.width, rect.height);
}
///

inline clock_t millis_as_clock(uint64_t millis) {
    return (clock_t) ((millis * CLOCKS_PER_SEC) / 1000);
}

Cooldown cooldown_new(uint64_t millis) {
    return (Cooldown) {
        .duration = millis_as_clock(millis), 
        .then = clock(),
    };
}

bool cooldown_done(Cooldown* c) {
    clock_t now = clock();
    bool done = now - c->then >= c->duration;

    if (done) c->then = now;

    return done;
}
