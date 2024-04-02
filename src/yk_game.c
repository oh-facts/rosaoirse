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
#define MAP_SIZE (3)

u32 map[MAP_SIZE * MAP_SIZE * MAP_SIZE];

typedef struct v3
{
    f32 x, y, z;
} v3;

typedef struct v2
{
    f32 x, y;
} v2;

typedef struct v3i
{
    i32 x, y, z;
} v3i;

inline v3 v3_x_s(v3 a, f32 b)
{
    a.x *= b;
    a.y *= b;
    a.z *= b;

    return a;
}

inline v3 v3_mul_v3(v3 a, v3 b)
{
    return (v3){a.x * b.x, a.y * b.y, a.z * b.z};
}

inline v3 v3_add_v3(v3 a, v3 b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;

    return a;
}

inline v3 v3_sub_v3(v3 a, v3 b)
{
    return (v3){a.x - b.x, a.y - b.y, a.z - b.z};
}

inline f32 length(v3 a)
{
    return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

inline v3 v3_abs(v3 a)
{
    return (v3){fabsf(a.x), fabsf(a.y), fabsf(a.z)};
}

inline v3 f32_to_v3(f32 a)
{
    return (v3){a, a, a};
}

inline v3 v3_comp_div(v3 a, v3 b)
{
    return (v3){a.x / b.x, a.y / b.y, a.z / b.z};
}

inline v3 v3_sign(v3 a)
{
    return (v3){(a.x > 0) - (a.x < 0), (a.y > 0) - (a.y < 0), (a.z > 0) - (a.z < 0)};
}

inline v3i v3_to_v3i(v3 a)
{
    return (v3i){a.x, a.y, a.z};
}

inline v3 v3i_to_v3(v3i a)
{
    return (v3){a.x, a.y, a.z};
}

inline v3 v3_mul_f(v3 a, f32 b)
{
    return (v3){a.x * b, a.y * b, a.z * b};
}

inline v3 v3_add_f(v3 a, f32 b)
{
    return (v3){a.x + b, a.y + b, a.z + b};
}

YK_API void yk_innit_game(struct YkPlatform *platform, struct offscreen_buffer *screen)
{
    v3 a;
    a.x = 2;

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

    for (i32 x = 0; x < MAP_SIZE; x++)
    {
        for (i32 y = 0; y < MAP_SIZE; y++)
        {
            for (i32 z = 0; z < MAP_SIZE; z++)
            {
                map[x + y * MAP_SIZE + z * MAP_SIZE * MAP_SIZE] = rand()%2;
            }
        }
    }

    game->display.width = 400;
    game->display.height = 300;
    game->display.pixels = push_array(&game->arena,
                                      u32, game->display.width * game->display.height);
}

i32 getVoxel(v3i pos)
{
    return map[pos.x + pos.y * MAP_SIZE + pos.z * MAP_SIZE * MAP_SIZE];
}

const int MAX_RAY_STEPS = 50;
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
    tspeed = 2.f * delta;

    v3 camDir = {0, 0, -1.f};

    for (i32 i = 0, h = game->display.height; i < h; i++)
    {
        for (i32 j = 0, w = game->display.width; j < w; j++)
        {
            v3 camPlaneU = {1, 0, 0};
            v3 camPlaneV = {0, h * 1.f / w, 0};

            v2 screenPos = {2 * j / w - 1, 2 * i / h - 1};

            // v3 rayDir = camDir + screenPos.x * camPlaneU + screenPos.y * camPlaneV;
            v3 rayDir = {0};
            {
                v3 temp = v3_x_s(camPlaneU, screenPos.x);
                v3 temp2 = v3_x_s(camPlaneV, screenPos.y);

                // camPlaneU.x *= screenPos.x;
                v3 temp3 = v3_add_v3(temp, temp2);
                rayDir = v3_add_v3(temp3, camDir);
            }

            v3 rayPos = {0, 0, -5};
            v3i mapPos = v3_to_v3i(rayPos);
            v3 deltaDist = v3_abs(v3_comp_div(f32_to_v3(length(rayDir)), rayDir));

            v3i rayStep = v3_to_v3i(v3_sign(rayDir));
            v3 sideDist;

            {
                v3 temp1 = v3_mul_v3(v3_sign(rayDir), v3_sub_v3(v3i_to_v3(mapPos), rayPos));
                v3 temp2 = v3_mul_f(v3_sign(rayDir), 0.5);
                v3 temp3 = v3_add_v3(temp1, temp2);
                v3 temp4 = v3_add_f(temp3,0.5f);
                sideDist = v3_mul_v3(temp4, deltaDist);
            }

            v3i mask = {0};

            for (int i = 0; i < MAX_RAY_STEPS; i++)
            {
                if (getVoxel(mapPos))
                    break;

                if (sideDist.x < sideDist.y)
                {
                    if (sideDist.x < sideDist.z)
                    {
                        sideDist.x += deltaDist.x;
                        mapPos.x += rayStep.x;
                        mask = (v3i){true, false, false};
                    }
                    else
                    {
                        sideDist.z += deltaDist.z;
                        mapPos.z += rayStep.z;
                        mask = (v3i){false, false, true};
                    }
                }
                else
                {
                    if (sideDist.y < sideDist.z)
                    {
                        sideDist.y += deltaDist.y;
                        mapPos.y += rayStep.y;
                        mask = (v3i){false, true, false};
                    }
                    else
                    {
                        sideDist.z += deltaDist.z;
                        mapPos.z += rayStep.z;
                        mask = (v3i){false, false, true};
                    }
                }
            }

            u32 color = 0;
            if (mask.x)
            {
                color = RED;
            }
            if (mask.y)
            {
                color = BLUE;
            }
            if (mask.z)
            {
                color = GREEN;
            }

            ((u32 *)game->display.pixels)[i * w + j] = color;
        }
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
