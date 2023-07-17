#pragma once

// Services that the game provides to the platform layer

// Services that the game provides to the platform layer.

// Needs timing, controller/keyboard input, bitmap buffer to use, sound buffer to use.

class Sound_Manager;

struct Game_Back_Buffer {
    void *memory;
    s32   width;
    s32   height;
    s32   pitch;
    s32   bpp; // BytesPerPixel
};

struct Sound_Output_Buffer { 
    s32  samples_per_second;
    s32  sample_count;
    s16 *samples;
};

struct Game_Memory {
    bool  valid;
    u64   persistent_memory_size;
    void *persistent_memory;
};

void game_update_and_render(Game_Memory *game_memory, Game_Back_Buffer *back_buffer);
void game_get_sound_samples(Game_Memory *game_memory, Sound_Manager *sound_manager);


struct Game_State { 
    s32 blue_offset;
    s32 green_offset;
    s32 tone_per_second;
};

#pragma pack(push, 1)
struct BMP_Header { 
    u16 file_type;        /* File type, always 4D42h ("BM") */
    u32 file_size;        /* Size of the file in bytes */
    u16 reserved_1;       /* Always 0 */
    u16 reserved_2;       /* Always 0 */
    u32 bitmap_offset;    /* Starting position of image data in bytes */
    u32 size;             /* Size of this header in bytes */
    s32 width;            /* Image width in pixels */
    s32 height;           /* Image height in pixels */
    u16 planes;           /* Number of color planes */
    u16 bits_per_pixel;   /* Number of bits per pixel */
    u32 compression;      /* Compression methods used */
    u32 size_of_bitmap;   /* Size of bitmap in bytes */
    s32 horz_resolution;  /* Horizontal resolution in pixels per meter */
    s32 vert_resolution;  /* Vertical resolution in pixels per meter */
    u32 colors_used;      /* Number of colors in the image */
    u32 colors_important; /* Minimum number of important colors */
    
    // Compression = 3 use these values instead of colour palette.
    u32 red_mask;   /* Mask identifying bits of red component */
    u32 green_mask; /* Mask identifying bits of green component */
    u32 blue_mask;  /* Mask identifying bits of blue component */
    u32 alpha_mask; /* Mask identifying bits of alpha component */};
#pragma pack(pop)

struct Bitmap { 
    char *file_name;
    s32 width; 
    s32 height;
    u8 *data;
};

/* // Column vector */
/* struct V2 {  */
/*     f32 x; */
/*     f32 y; */
/* }; */

// Column vector
struct V3 { 
    f32 x;
    f32 y;
    f32 z;
};

// Column vector
struct V4 {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};

