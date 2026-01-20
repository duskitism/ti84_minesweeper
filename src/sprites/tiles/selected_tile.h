#ifndef selected_tile_include_file
#define selected_tile_include_file

#ifdef __cplusplus
extern "C" {
#endif

#define selected_tile_width 16
#define selected_tile_height 16
#define selected_tile_size 258
#define selected_tile ((gfx_sprite_t*)selected_tile_data)
extern unsigned char selected_tile_data[258];

#ifdef __cplusplus
}
#endif

#endif
