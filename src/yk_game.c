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

    struct bitmap display;

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

#include <math.h>
#define MAP_SIZE (8)
#define TILE_SIZE (8)

YK_API void yk_innit_game(struct YkPlatform *platform, struct offscreen_buffer *screen)
{
    struct Game *game = platform->memory;

    arena_innit(&game->arena, platform->mem_size - sizeof(struct Game), (u8 *)platform->memory + sizeof(struct Game));
    arena_innit(&game->scratch, platform->scratch_size, platform->scratch);

    u32 *pixel = screen->pixels;
    for (u32 i = 0; i < screen->height; i++)
    {
        for (u32 j = 0; j < screen->width; j++)
        {
            *pixel++ = 0xFF << 24 | lcg_rand() % 256 << 16 | lcg_rand() % 256 << 8 | lcg_rand() % 256;
        }
    }

    u8 *file_data = platform->read_file("../res/walker.bmp", &game->scratch);
    game->rabbit = make_bmp_from_file(file_data, &game->arena);

    game->display.width = 400;
    game->display.height = 300;
    game->display.pixels = push_array(&game->arena,
                                      u32, game->display.width * game->display.height);
}

YK_API void yk_update_and_render_game(struct YkPlatform *platform, struct offscreen_buffer *screen, struct YkInput *input, f32 delta)
{
    struct Game *game = (struct Game *)platform->memory;

    draw_rect(&game->display, 0, 0, game->display.width, game->display.height, 0xFFAA22FF);

#define W_2 200
#define Y_2 150
#define DIRX cos(0)
#define DIRY sin(0)
    local_persist f32 posx = W_2, posy = Y_2;
    local_persist f32 speed;
    speed = 10 * delta;

    local_persist f32 dirx, diry;
    local_persist f32 angle = 0;
    local_persist f32 tspeed;
    tspeed = 0.5f * delta;

    dirx = cos(angle);
    diry = sin(angle);

    if (yk_input_is_key_held(input, YK_KEY_W))
    {
        posx += speed * dirx;
        posy += speed * diry;
    }
    if (yk_input_is_key_held(input, YK_KEY_A))
    {
        angle -= tspeed;
    }
    if (yk_input_is_key_held(input, YK_KEY_D))
    {
        angle += tspeed;
    }

    if (yk_input_is_key_held(input, YK_KEY_RIGHT))
    {
        f32 x1 = 250;
        f32 y1 = 120;
        f32 x2 = 250;
        f32 y2 = 180;

        draw_line(&game->display, x1, y1, x2, y2, GREEN);
        // player
        draw_line(&game->display, posx, posy, posx + dirx * 5, posy + diry * 5, WHITE);
    }
    if (yk_input_is_key_held(input, YK_KEY_LEFT))
    {
        f32 x1 = 250 - posx;
        f32 y1 = 120 - posy;
        f32 x2 = 250 - posx;
        f32 y2 = 180 - posy;

        f32 tz1 = x1 * cos(angle) + y1 * sin(angle);
        f32 tz2 = x2 * cos(angle) + y2 * sin(angle);

        f32 tx1 = x1 * sin(angle) - y1 * cos(angle);
        f32 tx2 = x2 * sin(angle) - y2 * cos(angle);

        draw_line(&game->display, W_2 - tx1, Y_2 - tz1, W_2 - tx2, Y_2 - tz2, GREEN);
        // player
        draw_line(&game->display, W_2, Y_2, W_2, Y_2 + 5, WHITE);
    }
    if (yk_input_is_key_held(input, YK_KEY_DOWN))
    {

        f32 x1 = 250 - posx;
        f32 y1 = 120 - posy;
        f32 x2 = 250 - posx;
        f32 y2 = 180 - posy;

        f32 tz1 = x1 * cos(angle) + y1 * sin(angle);
        f32 tz2 = x2 * cos(angle) + y2 * sin(angle);

        f32 tx1 = x1 * sin(angle) - y1 * cos(angle);
        f32 tx2 = x2 * sin(angle) - y2 * cos(angle);

        x1 = -tx1 * 16 / tz1;
        f32 y1a = -Y_2 / tz1;
        f32 y1b = Y_2 / tz1;

        x2 = -tx2 * 16 / tz2;
        f32 y2a = -Y_2 / tz2;
        f32 y2b = Y_2 / tz2;

        draw_line(&game->display, W_2 + x1, Y_2 + y1a, W_2 + x2, Y_2 + y2a, RED);
        draw_line(&game->display, W_2 + x1, Y_2 + y1b, W_2 + x2, Y_2 + y2b, GREEN);
        draw_line(&game->display, W_2 + x1, Y_2 + y1a, W_2 + x1, Y_2 + y1b, BLUE);
        draw_line(&game->display, W_2 + x2, Y_2 + y2a, W_2 + x2, Y_2 + y2b, BLACK);
    }

    struct bitmap bmp = {0};
    bmp.pixels = screen->pixels;
    bmp.height = screen->height;
    bmp.width = screen->width;

    struct render_rect rect = {0};
    rect.w = game->display.width;
    rect.h = game->display.height;
    rect.x = 0;
    rect.y = 0;

    blit_bitmap_scaled(&bmp, &game->display, &rect, 0, 0, 2, 2);
}
