#ifndef bomb_include_file
#define bomb_include_file

#ifdef __cplusplus
extern "C" {
#endif

#define bomb_width 32
#define bomb_height 32
#define bomb_size 1026
#define bomb ((gfx_sprite_t*)bomb_data)
extern unsigned char bomb_data[1026];

#ifdef __cplusplus
}
#endif

#endif
