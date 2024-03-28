#include <yk_renderer.h>
#include <platform/yk_platform.h>

struct Game
{
    struct Arena arena;
    struct Arena scratch;
    
    struct bitmap rabbit;
};

internal b8 yk_input_is_key_tapped(struct YkInput *state, u32 key)
{
    return state->keys[key] && !state->keys_old[key];
}

internal b8 yk_input_is_key_held(struct YkInput *state, u32 key)
{
    return state->keys[key];
}


YK_API void yk_innit_game(struct YkPlatform *platform, struct offscreen_buffer *screen)
{   
    struct Game* game = platform->memory;
    
    arena_innit(&game->arena, platform->mem_size - sizeof(struct Game), (u8*)platform->memory + sizeof(struct Game));
    arena_innit(&game->scratch, platform->scratch_size, platform->scratch);
    
    u32 *pixel = screen->pixels;
    for (u32 i = 0; i < screen->height; i++)
    {
        for (u32 j = 0; j < screen->width; j++)
        {
            *pixel++ = 0xFF << 24 | rand() % 256 << 16 | rand() % 256 << 8 | rand() % 256;
        }
    }
    
    u8* file_data = platform->read_file("../res/spritesheet.bmp", &game->scratch);
    game->rabbit = make_bmp_from_file(file_data,&game->arena);
    
    struct bitmap bmp = {0};
    bmp.pixels = screen->pixels;
    bmp.height = screen->height;
    bmp.width = screen->width;
    
    struct render_rect dst_rect = {0};
    dst_rect.w = screen->width;
    dst_rect.h = screen->height;
    dst_rect.x = 0;
    dst_rect.y = 0;
    
    struct render_rect src_rect = {0};
    src_rect.w = game->rabbit.width/4;
    src_rect.h = game->rabbit.height/3;
    
    for(u32 i = 0; i < 4; i ++)
    {
        for(u32 j = 0; j < 3; j ++)
        {
            src_rect.x = j*16;
            src_rect.y = i*16;
            
            blit_bitmap(&bmp,&game->rabbit, &dst_rect, &src_rect);
            dst_rect.x += 16;
        }
    }
}

YK_API void yk_update_and_render_game(struct YkPlatform *platform, struct offscreen_buffer *screen, struct YkInput *input, f32 delta)
{
    struct Game* game = platform->memory;
    
    local_persist f32 dirx = 0;
    if(yk_input_is_key_held(input,YK_ACTION_UP))
    {
        dirx += delta * 50;
        platform->set_title(platform->_win,"gelato");
    }
    
    local_persist f32 timer = 0;
    timer += delta;
    
    printl("%f",timer);
    
    struct bitmap bmp = {0};
    bmp.pixels = screen->pixels;
    bmp.height = screen->height;
    bmp.width = screen->width;
    
    struct render_rect dst_rect = {0};
    dst_rect.w = screen->width;
    dst_rect.h = screen->height;
    dst_rect.x = dirx;
    dst_rect.y = 0;
    
    struct render_rect src_rect = {0};
    src_rect.w = game->rabbit.width/4;
    src_rect.h = game->rabbit.height/3;
    
    for(u32 i = 0; i < 4; i ++)
    {
        for(u32 j = 0; j < 3; j ++)
        {
            src_rect.x = j*16;
            src_rect.y = i*16;
            
            blit_bitmap(&bmp,&game->rabbit, &dst_rect, &src_rect);
            dst_rect.x += 16;
        }
    }
    
    
}