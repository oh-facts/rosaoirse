/* date = March 21st 2024 4:28 pm */

#ifndef YK_PLATFORM_H
#define YK_PLATFORM_H
#include <yk_common.h>

struct offscreen_buffer
{
    u32 *pixels;
    i32 width;
    i32 height;
};

enum YK_KEY
{
    YK_KEY_HOLD_HANDS = 0,
    YK_KEY_UP,
    YK_KEY_LEFT,
    YK_KEY_RIGHT,
    YK_KEY_DOWN,
    YK_KEY_SPACE,
    YK_KEY_W,
    YK_KEY_A,
    YK_KEY_S,
    YK_KEY_D,
    
    
    YK_KEY_ACCEPT,
    YK_KEY_SAVE,
    YK_KEY_RESTORE,
    
    YK_KEY_COUNT,
};

typedef enum YK_KEY YK_KEY;

struct YkInput
{
    b8 keys[YK_KEY_COUNT];
    b8 keys_old[YK_KEY_COUNT];
};

struct YkPlatform
{
    void* memory;
    void* scratch; 
    
    size_t mem_size;
    size_t scratch_size;
    
    //platform
    void * _win;
    u32  (*innit_audio)(const char* path);
    void (*play_audio)(u32 audio_id);
    void (*stop_audio)(u32 audio_id);
    void (*set_title)(void * win, const char* title);
    u8*  (*read_file)(const char* filename, struct Arena* arena);
    
};


typedef void (*yk_innit_game_func)(struct YkPlatform *platform, struct offscreen_buffer *screen);
typedef void (*yk_update_and_render_game_func)( struct YkPlatform *platform, struct offscreen_buffer *screen, struct YkInput *input, f32 delta);

#ifdef YK_PLATFORM_DESKTOP
// Common functions for windows, mac and linux. Uses either the standard 
// library or cross platform libraries

/*
fopen_s() is only defined in windows
*/
// thank you gruelingpine185 for correcting this
#if defined(__unix__) || defined(__APPLE__)
#define fopen_s(pFile, filepath, mode) ((*(pFile)) = fopen((filepath), (mode))) == NULL
#endif

/*
Haven't tested it. Wrote it in a different engine. Modified it to use the arena
*/
internal u8 *yk_read_text_file(const char *filepath, struct Arena *arena)
{
    FILE *file;
    fopen_s(&file, filepath, "r");
    
    AssertM(file, "Unable to open the file %s\n", filepath);
    
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    AssertM(length > 0, "File %s is empty", filepath);
    
    u8 *string = push_array(arena, char, length + 1);
    
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
internal int yk_clone_file(const char *sourcePath, const char *destinationPath)
{
    FILE *sourceFile, *destinationFile;
    char buffer[4096];
    size_t bytesRead;
    
    fopen_s(&sourceFile, sourcePath, "rb");
    
    AssertM(sourceFile, "Unable to open the file %s\n", sourcePath);
    
    fopen_s(&destinationFile, destinationPath, "wb");
    
    AssertM(destinationFile, "Unable to open the file %s\n", destinationPath);
    
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0)
    {
        fwrite(buffer, 1, bytesRead, destinationFile);
    }
    
    fclose(sourceFile);
    fclose(destinationFile);
    
    return 1;
}

// Tested. Works.
internal u8 *yk_read_binary_file(const char *filename, struct Arena *arena)
{
    FILE *file;
    fopen_s(&file, filename, "rb");
    
    AssertM(file,"Failed to open file %s",filename);
    
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    u8 *buffer = push_array(arena, u8, fileSize);
    
    if (fread(buffer, 1, fileSize, file) != fileSize)
    {
        AssertM(0,"Failed to read file %s",filename);
    }
    
    fclose(file);
    
    return buffer;
}

// Until I Do audio properly, I am just putting it here
#include <miniaudio/miniaudio.h>

global ma_result result;
global ma_engine engine;

global ma_sound sounds[10];
global u32 num_sounds;

internal u32 miniaudio_innit_audio(const char *path)
{
    result = ma_sound_init_from_file(&engine, path, 0, NULL, NULL, &sounds[num_sounds]);
    if (result != MA_SUCCESS)
    {
        printf("Audio %s not found", path);
    }
    return num_sounds++;
}

internal void miniaudio_play_audio(u32 id)
{
    ma_sound_start(&sounds[id]);
}

internal void miniaudio_stop_audio(u32 id)
{
    ma_sound_stop(&sounds[id]);
    ma_sound_seek_to_pcm_frame(&sounds[id], 0);
}

#endif

#endif //YK_PLATFORM_H
