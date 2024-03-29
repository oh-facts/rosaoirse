#include <yk_renderer.h>
#include <stb/stb_truetype.h>

/*
    :vomit:
    my alpha blending was a sin so even though I require an alpha channel. I am not going to use it
*/
void draw_rect(struct bitmap* dst, i32 minx, i32 miny, i32 maxx, i32 maxy, u32 rgba)
{
    if(maxx < 0 || maxy < 0  || minx > dst->width || miny > dst->height)
    {
        return;
    }
    
    minx = (minx < 0) ? 0 : minx;
    miny = (miny < 0) ? 0 : miny;
    maxx = (maxx > dst->width) ? dst->width : maxx;
    maxy = (maxy > dst->height) ? dst->height : maxy;
    
    // ToDo(facts): store pitch inside offscreen_buffer
    u8 *row = (u8 *)dst->pixels + miny * (dst->width * 4) + minx * 4;
    
    for (u32 y = miny; y < maxy; y++)
    {
        u32 *pixel = (u32 *)row;
        
        for (u32 x = minx; x < maxx; x++)
        {
            if(((rgba >> 24) & 0xFF) < 255)
            {
                continue;
            }
            
            *pixel++ = rgba;
        }
        row += dst->width * 4;
    }
}

void blit_bitmap(struct bitmap* dst, struct bitmap* src, struct render_rect* src_rect, i32 posx, i32 posy)
{
    
    for(u32 y = 0; y < src_rect->h; y++)
    {
        u32* pixel = (u32*)src->pixels + (src_rect->y + y) * src->width + src_rect->x;
        
        for(u32 x = 0; x < src_rect->w; x++)
        {
            draw_rect(dst,
                      x + posx,
                      y + posy,
                      x + posx + 1,
                      y + posy + 1,
                      *pixel++);
        }
    }
}
void blit_bitmap_scaled(struct bitmap* dst, struct bitmap* src, struct render_rect* src_rect, i32 posx, i32 posy, f32 scalex, f32 scaley)
{
    for(u32 y = 0; y < src_rect->h; y++)
    {
        u32* pixel = (u32*)src->pixels + (src_rect->y + y) * src->width + src_rect->x;
        
        i32 scaledy = scaley*y + posy;
        
        for(u32 x = 0; x < src_rect->w; x++)
        {
            i32 scaledx = scalex*x + posx;
            
            draw_rect(dst,
                      scaledx,
                      scaledy,
                      scaledx + scalex,
                      scaledy + scaley,
                      *pixel++);
        }
    }
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

struct bitmap make_bmp_from_file(u8* file_data, struct Arena* arena)
{
    u32 * out = 0;
    
    struct BitmapHeader* header = (struct BitmapHeader*)file_data;
    AssertM(header->depth == 32, "your bitmap must use 32 bytes for each pixel");
    
    struct bitmap result = {0};
    result.width = header->width;
    result.height = header->height;
    
    result.pixels = push_array(arena,u32,result.width*result.height);
    
    memcpy(result.pixels, (u8*)file_data + header->pixel_offset,result.width * result.height * sizeof(u32));
    
    
    // erm, there has to be a better way to flip pixels right?
    u32 temp;
    for (size_t y = 0, yy = result.height; y < yy / 2; y++)
    {
        for (size_t x = 0, xx = result.width; x < xx ; x++)
        {
            temp = ((u32*)result.pixels)[y * xx + x];
            ((u32*)result.pixels)[y * xx + x] = ((u32*)result.pixels)[(yy- 1 - y) * xx + x];
            ((u32*)result.pixels)[(yy - 1 - y) * xx + x] = temp;
        }
    }
    
    
    return result;
}

struct bitmap make_bmp_font(u8* file_data, char codepoint,  struct Arena* arena)
{
    
    struct bitmap out = {0};
    stbtt_fontinfo font;
    stbtt_InitFont(&font, (u8*)file_data, stbtt_GetFontOffsetForIndex((u8*)file_data,0));
    
    i32 w,h,xoff,yoff;
    u8* bmp = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, 100), codepoint ,&w,&h, &xoff, &yoff);
    
    out.width = w;
    out.height = h;
    out.pixels = push_array(arena,u32,w * h);
    
    u8* src = bmp;
    u8* dest_row = (u8*)out.pixels;
    
    for(u32 y = 0; y < h; y ++)
    {
        u32* dest = (u32*)dest_row;
        for(u32 x = 0; x < w; x ++)
        {
            u8 alpha = *src++;
            *dest++ = ((alpha <<24) |
                       (alpha <<16) |
                       (alpha << 8) |
                       (alpha ));
        }
        dest_row += 4 * out.width;
    }
    
    stbtt_FreeBitmap(bmp, 0);
    return out;
}

void set_pixel(struct bitmap* dst, f32 x, f32 y, u32 color)
{
    i32 height = dst->height;
    i32 width = dst->width;
    
    if(x > width || y > height || x <= 0 || y <= 0)
    {
        return;
    }
    
    u32* pixel = dst->pixels;
    pixel[(i32)y * width + (i32)x] = color;
}

#include <math.h>

// wikipedia dda
void draw_line(struct bitmap* dst, f32 x1, f32 y1, f32 x2, f32 y2, u32 color)
{
    f32 dx = x2 - x1;
    f32 dy = y2 - y1;
    
    f32 step = fabsf(dx) >= fabsf(dy) ? fabsf(dx) : fabsf(dy);
    
    dx = dx/step;
    dy = dy/step;
    
    f32 x = x1;
    f32 y = y1;
    
    i32 i = 0;
    
    while( i <= step)
    {
        set_pixel(dst,x,y,color);
#if 0
        // I have lost my mind
        // I think it would be better to render to a smaller bmp, then scale
        // the bmp. This is pyscho.
        set_pixel(dst,x + 1,y);
        set_pixel(dst,x,y + 1);
        set_pixel(dst,x - 1,y);
        set_pixel(dst,x,y - 1);
        set_pixel(dst,x + 1,y + 1);
        set_pixel(dst,x - 1,y - 1);
        set_pixel(dst,x + 1,y - 1);
        set_pixel(dst,x - 1,y + 1);
#endif
        //printf("%f %f\n",x,y);
        x += dx;
        y +=dy;
        i++;
    }
    
}