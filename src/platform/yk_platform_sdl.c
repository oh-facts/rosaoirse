#include <platform/yk_sdl2_include.h>
#include <miniaudio.h>
#include <yk_common.h>
#include <yk_game.h>

#include <sys/stat.h>
#include <time.h>

#define DEBUG_SDL_CHECK 1
#define LOG_STATS       0

#if DEBUG_SDL_CHECK
#define SDL_CHECK_RES(res) _Assert_helper(res == 0, "[SDL Assert is Failure]\n%s", SDL_GetError())
#define SDL_CHECK(expr) _Assert_helper(expr, "[SDL Assert is Failure]\n%s", SDL_GetError())
#else
#define SDL_CHECK_RES(expr) expr
#define SDL_CHECK(expr)
#endif


// ToDO: Haven't tested hot reloading on mac or linux.
#if     _WIN32
#define GAME_DLL        "yk2.dll"
#define GAME_DLL_CLONED "yk2_temp.dll"
#elif   __linux__
#define GAME_DLL        "libyk2.so"
#define GAME_DLL_CLONED "libyk2_temp.so"
#elif  __APPLE__        
#define GAME_DLL        "yk2.dylib"
#define GAME_DLL_CLONED "yk2_test.dylib"
#endif

internal char* yk_read_text_file(const char* filepath, struct Arena* arena);
internal int yk_clone_file(const char* sourcePath, const char* destinationPath);
internal char* yk_read_binary_file(const char* filename, struct Arena* arena);
internal time_t get_file_last_modified_time(const char* pathname);

struct sdl_platform
{
    yk_innit_game_func innit_game;
    yk_update_and_render_game_func update_and_render_game;
    void * game_dll;
    
    // unused
    time_t modified_time;
};

internal void load_game_dll(struct sdl_platform* platform)
{
    yk_clone_file(GAME_DLL, GAME_DLL_CLONED);
    
    platform->game_dll = SDL_LoadObject(GAME_DLL_CLONED);
    if (!platform->game_dll)
    {
        exit(432);
        
    }
    yk_innit_game_func innit_game = NULL;
    yk_update_and_render_game_func update_and_render_game = NULL;
    
    platform->innit_game = (yk_innit_game_func)SDL_LoadFunction(platform->game_dll, "yk_innit_game");
    platform->update_and_render_game = (yk_update_and_render_game_func)SDL_LoadFunction(platform->game_dll, "yk_update_and_render_game");
    platform->modified_time = get_file_last_modified_time(GAME_DLL);
}

internal void hot_reload(struct sdl_platform* platform)
{
    SDL_UnloadObject(platform->game_dll);
    load_game_dll(platform);
}

// sys/stat works on windows because its a compatibility thing. I don't know how safe this is. This function works as expected on my computer so it will stay. Also, this is a debug tool. I won't ship with this, so it will stay.
internal time_t get_file_last_modified_time(const char* pathname)
{
    struct stat stat_buf;
    if(stat(pathname,&stat_buf) == -1 )
    {
        printl("could not get last modified time. The dev needs to write more info here to make this error message remotely helpful");
        return 0;
    }
    
    return stat_buf.st_mtime;
}

//https://en.wikipedia.org/wiki/BMP_file_format
#pragma pack(push, 1)
struct BitmapHeader
{
    //file header
    u16 file_type;
    u32 file_size;
    u16 _reserved;
    u16 _reserved2;
    u32 pixel_offset;
    
    //DIB header
    u32 header_size;
    i32 width;
    i32 height;
    u16 color_panes;
    u16 depth;// stored in bits
};
#pragma pack(pop)


struct bitmap
{
    u32* pixels;
    u32 width;
    u32 height;
};

// Note(facts): This is stored upside down. So I flip it (along x axis)
internal struct bitmap read_bitmap_file(const char* filepath, struct Arena* arena)
{
    u32 * out = 0;
    
    size_t arena_save = arena->used;
    char* file_data = yk_read_binary_file(filepath, arena);
    
    struct BitmapHeader* header = (struct BitmapHeader*)file_data;
    AssertM(header->depth == 32, "your bitmap must use 32 bytes for each pixel");
    
    struct bitmap result;
    result.width = header->width;
    result.height = header->height;
    
    arena->used = arena_save;
    
    result.pixels = push_array(arena,u32,result.width*result.height);
    
    memcpy(result.pixels, (u8*)file_data + header->pixel_offset,result.width * result.height * sizeof(u32));
    
    // erm, there has to be a better way to flip pixels right?
    u32 temp;
    for (size_t y = 0, yy = result.height; y < yy / 2; y++)
    {
        for (size_t x = 0, xx = result.width; x < xx ; x++)
        {
            temp = result.pixels[y * xx + x];
            result.pixels[y * xx + x] = result.pixels[(yy- 1 - y) * xx + x];
            result.pixels[(yy - 1 - y) * xx + x] = temp;
        }
    }
    
    
    return result;
}


#define GAME_UPDATE_RATE (1/60.f)

ma_result result;
ma_engine engine;


// ToDO(facts): Do properly loser
void sdl_play_audio(const char* path)
{
    ma_engine_play_sound(&engine, path, 0);
}

void sdl_set_title(void* win, const char* title)
{
    // incase text doesn't fully display
    printf("%s\n",title);
    SDL_SetWindowTitle(win, title);
}

#if _WIN32
#include <Windows.h>
#endif

int main(int argc, char *argv[])
{
    // FUCK MICROSOFT (John Malkovitch voice)
#if _WIN32
    SetProcessDPIAware();
#endif
    
    // platform shit starts here ------------------------
    struct sdl_platform platform = {0};
    load_game_dll(&platform);
    
    f64 total_time_elapsed = 0;
    f64 dt = 0;
    
    SDL_CHECK_RES(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO));
    
    if(TTF_Init() != 0)
    {
        printf("%s",SDL_GetError());
    }
    
    const u32 tv_w  = 8;
    const u32 tv_h  = 6;
    const u32 pad_w = 4; // 4 on each side
    const u32 pad_h = 3;
    const u32 win_w = tv_w + 2 * pad_w;
    const u32 win_h = tv_h + pad_h;
    
    SDL_Window *win = SDL_CreateWindow(
                                       "television",
                                       SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 960, 540,
                                       SDL_WINDOW_RESIZABLE);
    
    SDL_CHECK(win);
    
    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        return -1;
    }
    
    SDL_Surface *win_surf = SDL_GetWindowSurface(win);
    SDL_CHECK(win_surf);
    
    SDL_Event event;
    b8 quit = 0;
    
    //-----------------------------platform shit ends here
    
    // game boilerplate
    struct YkInput input = {0};
    
    struct YkGame game = {0};
    
    size_t mem_size = Megabytes(10);
    arena_innit(&game.arena,mem_size,malloc(mem_size));
    //platform
    game._win = win;
    game.platform_play_audio = sdl_play_audio;
    game.platform_set_title = sdl_set_title;
    
    platform.innit_game(&game);
    
    struct render_buffer render_target = {0};
    // SDL_GetWindowSize(win, &render_target.height, &render_target.width);
    render_target.height = 60;
    render_target.width = 80;
    render_target.pixels = push_array(&game.arena, sizeof(u32),render_target.height * render_target.width);
    
    SDL_Surface *render_surface = SDL_CreateRGBSurfaceFrom(render_target.pixels, render_target.width, render_target.height, 32, render_target.width * sizeof(u32), 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
    
    TTF_Font* font = TTF_OpenFont("../res/Delius-Regular.ttf",48);
    
    if(!font)
    {
        printf("%s",SDL_GetError());
    }
    SDL_Surface* text = TTF_RenderText_Solid(font, "dear dear", (SDL_Color){255,0,0});
    SDL_Surface* text2 = TTF_RenderText_Solid(font, "loves you", (SDL_Color){255,0,0});
    if(!text)
    {
        printf("%s",SDL_GetError());
    }
    
    
    struct bitmap bmp = read_bitmap_file("../res/test.bmp",&game.arena);
    
#if 0
    //Dangerous! Only call if its a small image. You won't be able to ctrl + c out of this if its millions of pixels
    for(u32 y = 0; y < bmp.height; y ++)
    {
        for(u32 x = 0; x < bmp.width; x ++)
        {
            printf("%x ",bmp.pixels[y*bmp.width + x]);
        }
        printl("");
    }
#endif
    SDL_Surface *bmo = SDL_CreateRGBSurfaceFrom(bmp.pixels, bmp.width, bmp.height, 32, bmp.width * sizeof(u32), 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
    
    SDL_DisplayMode dm;
    
    u8 isF = 0;
    
    i32 width, height;
    SDL_GetWindowSize(win, &width, &height);
    
    while (!quit)
    {
        time_t modified_time = get_file_last_modified_time(GAME_DLL);
        
        if(modified_time > platform.modified_time)
        {
            hot_reload(&platform);
        }
        
        f64 last_time_elapsed = total_time_elapsed;
        
        // ToDo(facts): Move this into a function. I dont want to scroll so much
        
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                {
                    quit = 1;
                }break;
                
                case SDL_KEYUP:
                case SDL_KEYDOWN:
                {
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_UP:
                        case SDLK_w:
                        {
                            input.keys[YK_ACTION_UP] = event.type == SDL_KEYDOWN ? 1 : 0;
                        }break;
                        
                        case SDLK_LEFT:
                        case SDLK_a:
                        {
                            input.keys[YK_ACTION_LEFT] = event.type == SDL_KEYDOWN ? 1 : 0;
                        }break;
                        
                        case SDLK_DOWN:
                        case SDLK_s:
                        {
                            input.keys[YK_ACTION_DOWN] = event.type == SDL_KEYDOWN ? 1 : 0;
                        }break;
                        
                        case SDLK_RIGHT:
                        case SDLK_d:
                        {
                            input.keys[YK_ACTION_RIGHT] = event.type == SDL_KEYDOWN ? 1 : 0;
                        }break;
                        
                        case SDLK_F1:
                        {
                            input.keys[YK_ACTION_HOLD_HANDS] = event.type == SDL_KEYDOWN ? 1 : 0;
                        }break;
                        case SDLK_RETURN:
                        {
                            input.keys[YK_ACTION_ACCEPT] = event.type == SDL_KEYDOWN ? 1 : 0;
                        }break;
                        
                        case SDLK_F2:
                        {
                            if (event.type == SDL_KEYDOWN)
                            {
                                if (!isF)
                                {
                                    isF = 1;
                                    SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
                                    
                                }
                                else
                                {
                                    isF = 0;
                                    SDL_SetWindowFullscreen(win, 0);
                                    SDL_SetWindowSize(win,960,540);
                                    SDL_GetWindowSize(win, &width, &height);
                                    SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                                    
                                }
                            }
                        }break;
                        
                        case SDLK_q:
                        {
                            quit = 1;
                        }break;
                        
                        case SDLK_1:
                        {
                            input.keys[YK_ACTION_SAVE] = event.type == SDL_KEYDOWN ? 1 : 0;
                        }break;
                        
                        case SDLK_2:
                        {
                            input.keys[YK_ACTION_RESTORE] = event.type == SDL_KEYDOWN ? 1 : 0;
                        }break;
                        
                        case SDLK_3:
                        {
                            hot_reload(&platform);
                        }break;
                        
                        default:
                        {
                            
                        }break;
                    }
                    
                }break;
                
                //ToDO:width resizing does not work
                case SDL_WINDOWEVENT:
                {
                    switch (event.window.event)
                    {
                        case SDL_WINDOWEVENT_RESIZED:
                        {
                            win_surf = SDL_GetWindowSurface(win);
                            SDL_CHECK(win_surf);
                            SDL_GetDesktopDisplayMode(0, &dm);
                            SDL_GetWindowSize(win, &width, &height);
                            
                            if(width < 800)
                            {
                                width = 800;
                                SDL_SetWindowSize(win, width, height);
                            }
                            
                        }break;
                    }
                }break;
                
                
            }
        }
        
        // ToDo(facts): store pitch inside render_buffer
        
        // game loop start--------
        local_persist f32 fixed_dt;
        fixed_dt += dt;
        
        if (fixed_dt > 1 / 60.f)
        {
            //render_target.pixels[0] = 0xFFFF0000;
            
            platform.update_and_render_game(&render_target, &input, &game, GAME_UPDATE_RATE);
            fixed_dt = 0;
            
            {
                
                //Note(facts): Not to happy with this. Move it to one of the events
                win_surf = SDL_GetWindowSurface(win);
                
                
                //tv render
                SDL_Rect ren_rect;
                
                ren_rect.h = height - (height /pad_h*1.f) * (0.6f);
                ren_rect.w = ren_rect.h * (tv_w*1.f/tv_h) ;
                ren_rect.x = (width - ren_rect.w)/2;
                ren_rect.y = 0;
                SDL_BlitScaled(render_surface, 0, win_surf, &ren_rect);
                
                //debug text 1
                SDL_Rect dstRect;
                dstRect.x = width/2 - text->w/2;    
                dstRect.y = ren_rect.h;    
                dstRect.w = 0;
                dstRect.h = 0;
                
                SDL_BlitSurface(text, 0, win_surf, &dstRect);
                
                //debug text 2
                dstRect.x = width/2 - text->w/2;  
                dstRect.y = ren_rect.h;
                dstRect.w = 0;
                dstRect.h = 0;
                
                SDL_BlitSurface(text2, 0, win_surf, &dstRect);
                
                
                /*
                                                                                                u32 tv_w = 8;
                                                                                    u32 tv_h = 6;
                                                                                    u32 pad_w = 4; // 4 on each side
                                                                                    u32 pad_h = 3;
                                                                                    u32 win_w = 16;
                                                                                    u32 win_h = 9;
                                                                                       */
            }
            
            // Might fail. Leaving it like this until it doesn't
            // It is possible that my window becomes invalid before
            // I recreate it. I could be wrong about this.
            // SDL_CHECK_RES(SDL_UpdateWindowSurface(win));
            
            // did fail. removed it.
            SDL_UpdateWindowSurface(win);
            
            for (u32 i = 0; i < YK_ACTION_COUNT; i++)
            {
                input.keys_old[i] = input.keys[i];
            }
        }
        
        //-------game loop end
        
        total_time_elapsed = SDL_GetTicks64() / 1000.f;
        
        dt = total_time_elapsed - last_time_elapsed;
        
        // perf stats
#if LOG_STATS
        
#define num_frames_for_avg 60
#define print_stats_time 5
        
        local_persist f64 frame_time;
        local_persist u32 frame_count;
        local_persist f32 time_since_print;
        
        frame_time += dt;
        frame_count++;
        time_since_print += dt;
        
        if (time_since_print > print_stats_time)
        {
            frame_time /= frame_count;
            f64 frame_rate = 1 / frame_time;
            
            printf("\n     perf stats     \n");
            printf("\n--------------------\n");
            
            printf("frame time : %.3f ms\n", frame_time * 1000.f);
            printf("frame rate : %.0f \n", frame_rate);
            
            printf("---------------------\n");
            
            frame_time = 0;
            frame_count = 0;
            
            time_since_print = 0;
        }
#endif
    }
    
#if mem_leak_msvc
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtDumpMemoryLeaks();
#endif
    return 0;
}

/*
fopen_s() is only defined in windows
*/
#ifdef __unix
#define fopen_s(pFile, filepath, mode) ((*(pFile)) = fopen((filepath), (mode))) == NULL
#endif
/*
Haven't tested it. Wrote it in a different engine. Modified it to use the arena
*/
internal char* yk_read_text_file(const char* filepath, struct Arena* arena)
{
    FILE* file;
    fopen_s(&file, filepath, "r");
    
    AssertM(file, "Unable to open the file %s\n",filepath);
    
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    AssertM(length > 0, "File %s is empty",filepath);
    
    char* string = push_array(arena, char, length + 1);
    
    char c;
    int i = 0;
    
    while ((c = fgetc(file)) != EOF)
    {
        string[i] = c;
        i++;
    }
    string[i] = '\0';
    
    fclose(file);
    
    return string;
}

// tested. works. will fail if dll is > 4kb. Currently it is 28 kb. (I am only using it for dll currently so I am using fat array).
internal int yk_clone_file(const char* sourcePath, const char* destinationPath)
{
    FILE* sourceFile, * destinationFile;
    char buffer[4096];
    size_t bytesRead;
    
    fopen_s(&sourceFile, sourcePath, "rb");
    if (sourceFile == NULL) {
        perror("Error opening source file");
        return 0;
    }
    
    fopen_s(&destinationFile, destinationPath, "wb");
    if (destinationFile == NULL) {
        perror("Error opening destination file");
        fclose(sourceFile);
        return 0;
    }
    
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0) {
        fwrite(buffer, 1, bytesRead, destinationFile);
    }
    
    fclose(sourceFile);
    fclose(destinationFile);
    
    return 1;
}

// Tested. Works.
internal char* yk_read_binary_file(const char* filename, struct Arena* arena)
{
    FILE* file;
    fopen_s(&file, filename, "rb");
    
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = push_array(arena,u8,fileSize);
    
    if (fread(buffer, 1, fileSize, file) != fileSize)
    {
        perror("Failed to read file");
        arena->used -= fileSize;
        fclose(file);
        return 0;
    }
    
    fclose(file);
    
    return buffer;
}