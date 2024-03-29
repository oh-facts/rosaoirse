#include <yk_renderer.h>
#include <yk_platform.h>

internal u32 g_seed = 42;
u32 lcg_rand()
{
    g_seed = 214013 * g_seed + 2531011;
    return (g_seed >> 16);
}

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
            *pixel++ = 0xFF << 24 | lcg_rand() % 256 << 16 | lcg_rand() % 256 << 8 | lcg_rand() % 256;
        }
    }
    
    u8* file_data = platform->read_file("../res/walker.bmp", &game->scratch);
    game->rabbit = make_bmp_from_file(file_data,&game->arena);
    
}

#include <math.h>

YK_API void yk_update_and_render_game(struct YkPlatform *platform, struct offscreen_buffer *screen, struct YkInput *input, f32 delta)
{
    
    struct Game* game = platform->memory;
    
    
    struct bitmap bmp = {0};
    bmp.pixels = screen->pixels;
    bmp.height = screen->height;
    bmp.width = screen->width;
    
    draw_rect(&bmp,0,0,screen->width,screen->height,0xFFAA22FF);
    
    struct render_rect src_rect = {0};
    src_rect.w = game->rabbit.width/4;
    src_rect.h = game->rabbit.height;
    
    local_persist f32 counter = 0;
    counter += delta;
    
    src_rect.x = ((i32)(counter*5.f)%4)*32;
    blit_bitmap_scaled(&bmp,&game->rabbit, &src_rect,screen->width/2 - src_rect.w*10/2,screen->height/2 - src_rect.h*10/2,10,10);
    
    local_persist f32 posx = 100;
    local_persist f32 posy = 100;
    local_persist f32 angle = 3.14 / 2;
    
    const f32 speed = 100 * delta;
    const f32 tspeed = 3 * delta;
    
    if(yk_input_is_key_held(input,YK_KEY_W))
    {
        posy += speed*sin(angle);
        posx += speed*cos(angle);
    }
    if(yk_input_is_key_held(input,YK_KEY_A))
    {
        angle -= tspeed;
    }
    if(yk_input_is_key_held(input,YK_KEY_D))
    {
        angle += tspeed;
    }
    
    
    i32 len = 40;
    draw_line(&bmp, posx,posy, posx + len* cos(angle),posy + len*sin(angle), GREEN);
    
    //exit(32);
    
}