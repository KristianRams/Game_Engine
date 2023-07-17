#pragma once 

struct Back_Buffer {
    BITMAPINFO info;
    void      *memory;
    s32        width;
    s32        height;
    s32        pitch;
    s32        bpp; // BytesPerPixel
};

struct Window_Dimension { 
    s32 width; 
    s32 height;
};

struct Sound_Output {
    s32 samples_per_second;
    u32 running_sample_index;
    s32 bytes_per_sample;
    s32 secondary_buffer_size;
    f32 t_sine;
    // How many samples ahead of the play cursor we would like to be.
};

