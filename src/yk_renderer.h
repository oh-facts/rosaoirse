/* date = March 20th 2024 11:18 pm */

#ifndef YK_RENDERER_H
#define YK_RENDERER_H

#include <yk_common.h>
#include <yk_arena.h>
#include <platform/yk_platform.h>

struct render_rect
{
    i32 x,y,w,h;
};


struct render_buffer
{
    u32 *pixels;
    i32 width;
    i32 height;
};


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

// Note(facts): This is stored upside down. So I flip it (along x axis)
YK_API struct bitmap read_bitmap_file(char* file_data, struct Arena* arena);

YK_API void blit_bitmap_scaled(struct render_buffer* dst, struct render_buffer* src, struct render_rect* dst_rect);

YK_API void blit_ttf(struct render_buffer* dst, i32 posx, i32 posy,  const char* text);

YK_API void draw_rect(struct render_buffer *screen, i32 minx, i32 miny, i32 maxx, i32 maxy, u32 rgba);

#endif //YK_RENDERER_H