// Semi one-filer because I was too lazy to split it off into headers
#include "hexa_lib/hexa_lib.h"
#include "sprites/all.h"

static bool FAILURE = false;
static unsigned CELL_SIZE;

#define MINE_COUNT 40
/// n * n grid dimensions
#define DIMENSIONS 16

#define for_3x3(p, q) for (int p = -1; p <= 1; ++p) \
        for (int q = -1; q <= 1; ++q) \

typedef enum {
    LightBlue = 0x1eu,
    NavyBlue = 0x33u,
    LightGreen = 0x8eu,
    DarkGreen = 0x44u,
    NatureGreen = 0x8du,
    Purple = 0x71u,
    Red = 0xe0u,
    SoftWhite = 0xfeu,
} ColorPallet;

// ---
#define GENERAL_SPRITE_QTY 5
#define NUMBER_SPRITE_QTY 8

typedef struct {
    gfx_sprite_t* general[GENERAL_SPRITE_QTY];
    gfx_sprite_t* numbers[NUMBER_SPRITE_QTY];
} Sprites;

typedef enum {
    Tile = 0,
    SelectedTile,
    Bomb,
    Flag,
    Clock,
} GeneralSprite;

Sprites sprites_new() {
    return (Sprites) {
        .general = { 0 },
        .numbers = { 0 },
    };
}

void sprites_alloc_general(Sprites* s) {
    const gfx_sprite_t* raw_general[GENERAL_SPRITE_QTY] = {
        tile, selected_tile, bomb, flag, s_clock,
    };

    for (int g = 0; g < GENERAL_SPRITE_QTY; ++g) {
        gfx_sprite_t* alloc = gfx_MallocSprite(CELL_SIZE, CELL_SIZE);
        if (!alloc) {
            FAILURE = true;
            return;
        }

        gfx_ScaleSprite(raw_general[g], alloc);
        s->general[g] = alloc;
    }
}

void sprites_alloc_numbers(Sprites* s) {
    // * ishowspeed looking at fingers *
    const gfx_sprite_t* raw_numbers[NUMBER_SPRITE_QTY] = {
        one, two, three, four, five, six, seven, eight
    };

    for (int n = 0; n < NUMBER_SPRITE_QTY; ++n) {
        gfx_sprite_t* alloc = gfx_MallocSprite(CELL_SIZE, CELL_SIZE);
        if (!alloc) {
            FAILURE = true;
            return;
        }

        gfx_ScaleSprite(raw_numbers[n], alloc);
        s->numbers[n] = alloc;
    }
}

void sprites_alloc(Sprites* s) {
    sprites_alloc_general(s);
    sprites_alloc_numbers(s);
};

void sprites_destroy_general(Sprites* s) {
    for (int g = 0; g < GENERAL_SPRITE_QTY; ++g) 
        free(s->general[g]); // free NULL is fine
}

void sprites_destroy_numbers(Sprites* s) {
    for (int n = 0; n < NUMBER_SPRITE_QTY; ++n) 
        free(s->numbers[n]);
}
void sprites_destroy(Sprites* s) {
    sprites_destroy_general(s);
    sprites_destroy_numbers(s);
}
// ---

typedef struct {
    bool revealed;
    bool flagged;
    unsigned int value;
} Cell;

enum { Mine = 9 };

// I'm not using inline because it fw the compiler for some reason
Cell cell_new() {
    return (Cell) {
        .revealed = false,
        .flagged = false,
        .value = 0,
    };
}

bool cell_is_mine(Cell *c) {
    return (c->value == Mine);
}

// ---

typedef struct {
    Cell data[DIMENSIONS][DIMENSIONS];
    IVec2 selected;
    Cooldown flagging_cd;
    unsigned reveal_count;
    int flag_count;
    bool dug_mine;
} Board;


Board board_new() {
    return (Board) {
        .data = { 0 },
        .selected = (IVec2) { 0, 0 },
        .flagging_cd = cooldown_new(250 /* ms */),
        .flag_count = MINE_COUNT,
        .dug_mine = false,
    };
}

void board_alloc(Board* b) {
    for (int i = 0; i < DIMENSIONS; ++i) 
        for (int j = 0; j < DIMENSIONS; ++j) 
            b->data[i][j] = cell_new();
}

Cell* board_at(Board* b, IVec2 pos) {
    return &b->data[pos.y][pos.x];
}

Cell* board_at_selected(Board* b) {
    return board_at(b, b->selected);
}

bool board_near_selected(Board* b, IVec2 pos) {
    for_3x3(i, j) {
        IVec2 sum = ivec2_add(b->selected, (IVec2) { j, i });
        if (ivec2_eq(sum, pos)) return true;
    }
    return false;
}

void board_spawn_mines(Board* b) {
    srandom(clock() % (clock_t) UINT32_MAX); // Seeder
    
    for (int m = 0; m < MINE_COUNT; ++m) {
        IVec2 new_mine_pos = (IVec2) { randInt(0, DIMENSIONS - 1), randInt(0, DIMENSIONS - 1) };

        while (
            cell_is_mine(board_at(b, new_mine_pos))
                || board_near_selected(b, new_mine_pos)
        ) 
        {
            new_mine_pos.x = randInt(0, DIMENSIONS - 1);
            new_mine_pos.y = randInt(0, DIMENSIONS - 1);
        }
        board_at(b, new_mine_pos)->value = Mine;
    }
}

void board_set_values(Board* b) {
    for (int i = 0; i < DIMENSIONS; ++i) 
        for (int j = 0; j < DIMENSIONS; ++j) {
            if (cell_is_mine(&b->data[i][j])) continue;
            for_3x3 (k, l) {
                if (
                    (!k && !l) 
                        || (i + k < 0 || i + k >= DIMENSIONS) 
                        || (j + l < 0 || j + l >= DIMENSIONS)
                    ) continue;

                b->data[i][j].value += cell_is_mine(&b->data[i + k][j + l]);
            }         
        } 
}

void board_init(Board* b) {
    board_spawn_mines(b);
    board_set_values(b);
}

void board_flag_toggle(Board* b) {
    Cell* selected = board_at_selected(b);
    if (!selected->revealed && cooldown_done(&b->flagging_cd)) {
        selected->flagged ^= true;
        b->flag_count += 1 - (2 * selected->flagged);
    }
}

void board_dfs(Board* b, IVec2 cur_pos) {
    bool out_of_bounds = cur_pos.x < 0 || cur_pos.x >= DIMENSIONS
        || cur_pos.y < 0 
        || cur_pos.y >= DIMENSIONS;
    if (out_of_bounds) return;
 
    Cell* curr = board_at(b, cur_pos);

    if (curr->revealed || cell_is_mine(curr)) return;
    curr->revealed = true;
    ++b->reveal_count;
    b->flag_count += curr->flagged;
    // We only want 0's at this point
    if (curr->value) return; 
    // This spreads all directions (including diagonals)
    for_3x3(i, j) {
        if (!i && !j) continue;
        board_dfs(b, ivec2_add(cur_pos, (IVec2) { j, i }));
    }
}

void board_reveal_chunk(Board* b) {
    Cell* selected = board_at_selected(b);

    if (cell_is_mine(selected)) { 
        selected->revealed = true; 
        b->dug_mine = true; 
        return; 
    }
    board_dfs(b, b->selected);
}

void board_reveal_mines(Board* b) {
    for (int i = 0; i < DIMENSIONS; ++i) 
        for (int j = 0; j < DIMENSIONS; ++j) {
            Cell* curr = &b->data[i][j];
            if (cell_is_mine(curr)) curr->revealed = true;
        } 
}
// --
typedef struct {
    unsigned elapsed;
    // Cooldown is a mini abstraction inside hexalib
    Cooldown time_incr;
} Stopwatch;

Stopwatch stopwatch_new() {
    return (Stopwatch) {
        .elapsed = 0,
        // We want to increment elapsed every second
        .time_incr = cooldown_new(1000)
    };
}

void stopwatch_update(Stopwatch* s) {
    const unsigned REPR_CAP = 999;

    if (s->elapsed < REPR_CAP && cooldown_done(&s->time_incr)) 
        ++s->elapsed;
}

//

typedef enum {
    Normal,
    Win,
    Loss,
} GameState;

typedef struct Game {
    GameState state;
    bool first_click;
    Board board;
    Stopwatch stopwatch;
    Sprites sprites;
} Game;

//
static const unsigned GRID_FRAME_WIDTH  = SCREEN_WIDTH * 955 / 1000 - SCREEN_WIDTH / 4;
static const unsigned GRID_FRAME_HEIGHT = SCREEN_HEIGHT / 30;

#define GRID_FRAME_AREA ivec2_new(SCREEN_WIDTH / 4, GRID_FRAME_HEIGHT), ivec2_new(GRID_FRAME_WIDTH, GRID_FRAME_WIDTH)
#define GRID_WIDTH ui_frame_inner(GRID_FRAME_AREA).width 

// This thing is also used to calculate CELL_SIZE
// probably way better ways to do what I naively did 
Rectangle ui_frame_inner(IVec2 at, IVec2 dims) {
    IVec2 inner_scale = ivec2_new(
        (dims.x * 925 /* 92.5% of og */) / 1000, 
        (dims.y * 925) / 1000 
    );

    IVec2 square_pos = ivec2_new(
        (dims.x / 2) + at.x - inner_scale.x / 2,
        (dims.y / 2) + at.y - inner_scale.y / 2
    );

    return rect_new_v(square_pos, inner_scale);
}

/// @return Inner frame dimensions (as a rect) to be used for calculations
Rectangle ui_frame(IVec2 at, IVec2 dims) {
    gfx_SetColor((ColorPallet) DarkGreen);
    gfx_FillTriangle( // fake depth effect
        at.x, at.y,
        at.x, at.y + dims.y,
        at.x + dims.x, at.y
    );
    gfx_SetColor((ColorPallet) LightGreen);
    gfx_FillTriangle(
        at.x, at.y + dims.y,
        at.x + dims.x, at.y,
        at.x + dims.x, at.y + dims.y 
    );
    Rectangle inner_rect = ui_frame_inner(at, dims);
    rect_draw_565(inner_rect, (ColorPallet) LightBlue);

    return inner_rect;
}


void ui_flagcount(struct Game* g) {
    /* These mystical, magical numbers were conceived in Illustrator
     the pivot adjustment feature was dang useful
     I'm not going to bother extracting them into variables
    */
    ui_frame(ivec2_new(16, 46), ivec2_new(55, 27));

    gfx_TransparentSprite(g->sprites.general[(GeneralSprite) Flag], 23, 53);

    gfx_SetTextFGColor((g->board.flag_count < 0) ? (ColorPallet) Red : Purple);
    gfx_SetTextXY(41, 56);

    gfx_PrintInt(g->board.flag_count, 2);
}

void ui_clock(struct Game* g) {
    ui_frame(ivec2_new(16, 80), ivec2_new(55, 27));
    gfx_TransparentSprite(g->sprites.general[(GeneralSprite) Clock], 23, 87);

    gfx_SetTextFGColor(0x71);
    gfx_SetTextXY(41, 90);

    gfx_PrintInt(g->stopwatch.elapsed, 3);
}

void ui_instructions(void) {
    const char* list[] = { 
        "[arws]:",
        "move", 
        "[entr]:",
        "reveal", 
        "[+]:",
        "flag",
        "[2nd]",
        "reset"
    };

    unsigned list_len = sizeof(list) / sizeof(char*);

    // Got to really dig into that operating system
    Rectangle frame = ui_frame(ivec2_new(16, 113), ivec2_new(55, list_len * os_FontGetHeight()));

    unsigned magic_offset = 2;
    for (unsigned i = 0; i < list_len; ++i) {
        gfx_SetTextFGColor(((i + 1) & 1) == 0 ? Purple : Red);
        gfx_PrintStringXY(list[i], frame.x + 4, (frame.y + 5) + i * (os_FontGetHeight() - magic_offset));
    }
}

// This is either rendered on loss or win
void ui_endgame(struct Game* g) {
    Rectangle banner = rect_new(134, 82, 136, 16);
    Rectangle major = rect_new(banner.x, banner.y + banner.height, banner.width, 57);

    rect_draw_565(banner, NavyBlue);
    rect_draw_565(major, SoftWhite);

    bool won = (g->state == Win);
    gfx_SetTextFGColor((won) ? DarkGreen : Red); // ??::??::?;
    gfx_PrintStringXY((won) ? "Victory!" : "Uh oh!", 180, 116);

    gfx_SetTextFGColor((ColorPallet) Purple);
    gfx_PrintStringXY("-> [2nd] to reset", 143, 116 + os_FontGetHeight());
}

// ---

Game game_new() {
    return (Game) {
        .state = (GameState) Normal,
        .first_click = false,
        .board = board_new(),
        .stopwatch = stopwatch_new(),
        .sprites = sprites_new(),
    };
}

void game_alloc(Game* g) {
    board_alloc(&g->board);
    sprites_alloc(&g->sprites);
}

void game_reset(Game* g) {
    IVec2 curr_sel_pos = g->board.selected;

    g->state = Normal;
    g->stopwatch = stopwatch_new();
    
    g->board = board_new();
    g->board.selected = curr_sel_pos;
    board_alloc(&g->board);
    g->first_click = false;
}

void game_destroy(Game* g) {
    sprites_destroy(&g->sprites);
}

void game_on_enter(Game* g) {
    if (!g->first_click) {
        board_init(&g->board);
        g->first_click = true;
    }
    Cell* selected = board_at_selected(&g->board);
    if (!selected->flagged) board_reveal_chunk(&g->board);
}

void game_parse_input(Game* g) {
    kb_Scan();

    if (kb_IsDown(kb_Key2nd)) game_reset(g);

    IVec2* selector = &g->board.selected;

    if (kb_IsDown(kb_KeyUp) && selector->y - 1 >= 0) --selector->y;
    if (kb_IsDown(kb_KeyDown) && selector->y + 1 < DIMENSIONS) ++selector->y;
    if (kb_IsDown(kb_KeyLeft) && selector->x - 1 >= 0) --selector->x;
    if (kb_IsDown(kb_KeyRight) && selector->x + 1 < DIMENSIONS) ++selector->x;

    if (g->state != Normal) return;

    if (kb_IsDown(kb_KeyEnter)) game_on_enter(g);
    if (kb_IsDown(kb_KeyAdd)) board_flag_toggle(&g->board);  
}



void game_render_grid(Game *g) {
    Rectangle grid = ui_frame(GRID_FRAME_AREA);

    for (int i = 0; i < DIMENSIONS; ++i) 
        for (int j = 0; j < DIMENSIONS; ++j) {
            Cell* curr = &g->board.data[i][j];
            unsigned tile_x = grid.x + CELL_SIZE * j, tile_y = grid.y + CELL_SIZE * i;

            bool is_selected = ivec2_eq(g->board.selected, (IVec2) { j, i });

            if (!curr->revealed) {
                gfx_TransparentSprite(
                    g->sprites.general[
                        (is_selected) ? (GeneralSprite) SelectedTile : Tile
                    ], 
                    tile_x, 
                    tile_y
                );
                if (curr->flagged) 
                    gfx_TransparentSprite(g->sprites.general[(GeneralSprite) Flag], tile_x, tile_y);
                
                continue;
            }
             if (is_selected) {
                gfx_SetColor(0x6d);
                gfx_Rectangle(tile_x, tile_y, CELL_SIZE, CELL_SIZE); // outline
            }
            if (curr->value && curr->value <= NUMBER_SPRITE_QTY) 
                gfx_TransparentSprite(g->sprites.numbers[curr->value - 1], tile_x, tile_y);
            if (cell_is_mine(curr))
                gfx_TransparentSprite(g->sprites.general[(GeneralSprite) Bomb], tile_x, tile_y);
        }
}

void game_render(Game *g) {
    rect_draw_565((Rectangle) { 0,0, SCREEN_WIDTH, SCREEN_HEIGHT }, NatureGreen); /* Background */
    game_render_grid(g);

    ui_flagcount(g);
    ui_clock(g);
    ui_instructions();
    
    if (g->state != Normal) ui_endgame(g);
}

void game_on_loss(Game* g) {
    g->state = (GameState) Loss;
    board_reveal_mines(&g->board);
}

void game_update_state(Game* g) {
    if (g->state != (GameState) Normal) return;
    stopwatch_update(&g->stopwatch);
    if (g->board.dug_mine) game_on_loss(g);

    const unsigned SAFE_COUNT = (DIMENSIONS * DIMENSIONS) - MINE_COUNT;
    if (g->board.reveal_count == SAFE_COUNT) g->state = Win;
}

void game_update(Game* g) {
    game_parse_input(g);
    game_update_state(g);
    game_render(g);
}


void game_init(Game* g) {
    game_alloc(g);

    while (!FAILURE && !kb_IsDown(kb_KeyClear)) {
        game_update(g);

        gfx_SwapDraw();
    }
}

void init_statics(void) {
    CELL_SIZE = (GRID_WIDTH / DIMENSIONS);
}

void init(void) {
    init_statics();
    gfx_Begin();

    {
        Game game = game_new();
        
        game_init(&game);
        game_destroy(&game);
    }

    gfx_End();
}

int main(void) {
    init();
    
    return 0;
}
