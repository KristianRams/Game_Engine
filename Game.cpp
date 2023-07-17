#include "Intrinsics.h"
#include "Game.h"
#include <stdio.h>
#include <sys/stat.h>

s64 read_file(char *file_path, void **data_return) {
    FILE *file = fopen(file_path, "r");

    if (!file) { return -1; }

    s32 descriptor = _fileno(file);

    struct _stat file_stats;
    s32 result = _fstat(descriptor, &file_stats);
    if (result == -1) { return -1; }
    
    s64 size = file_stats.st_size;

    if (size == 0) { return -1; }

    u8 *data = new u8[size];

    s32 return_code = fseek(file, 0, SEEK_SET);

    s32 success = fread((void *)data, 1, size, file);
    if (success < 1) {
        delete[] data;
        return -1;
    }

    *data_return = data;

    return size;
}

extern s32 X_OFFSET;
extern s32 Y_OFFSET;

/*
void update_sound(Sound_Output_Buffer *sound_buffer, s32 tone_per_second) {
    static f32 t_sine = 0;
    s16 tone_volume = 1000;
    s32 wave_period = sound_buffer->samples_per_second / tone_per_second;

    s16 *sample_output = sound_buffer->samples;

    for (DWORD sample_index = 0; sample_index < sound_buffer->sample_count; ++sample_index) {
        f32 sine_value   = sinf(t_sine);
        s16 sample_value = (s16)(sine_value * tone_volume);

        *sample_output++ = sample_value;
        *sample_output++ = sample_value;

        t_sine += 2.0f * PI * 1.0f / (f32)wave_period;
        if (t_sine > 2.0f * PI) {
            t_sine -= 2.0f * PI;
        }
    }
}
*/

// Rasterization
void draw_gradient(Game_Back_Buffer *game_back_buffer, s32 x_offset, s32 y_offset) {
    u8 *row = (u8 *)game_back_buffer->memory;
    for (s32 y = 0; y < game_back_buffer->height; ++y) {
        u32 *pixel = (u32 *)row;

        for (s32 x = 0; x < game_back_buffer->width; ++x) {

            // RR GG BB XX  | What we want
            // XX BB GG RR  | What it gets place in memory if we get what we want
            // XX RR GG BB  | What windows dev actually placed the memory as
            // BB GG RR XX  | What actually gets placed in memory given what windows devs wanted.
            // 0xBB GG RR XX in memory

            u8 blue  = (x + x_offset);
            u8 green = (y + y_offset);

            *pixel++ = (green << 8) | blue;
        }

        row += game_back_buffer->pitch;
    }
}

// Clear backing buffer to black
void clear_to_back(Game_Back_Buffer *game_back_buffer) {
    u8 *row = (u8 *)game_back_buffer->memory;
    for (s32 y = 0; y < game_back_buffer->height; ++y) {
        u32 *pixel = (u32 *)row;

        for (s32 x = 0; x < game_back_buffer->width; ++x) {

            // RR GG BB XX  | What we want
            // XX BB GG RR  | What it gets place in memory if we get what we want
            // XX RR GG BB  | What windows dev actually placed the memory as
            // BB GG RR XX  | What actually gets placed in memory given what windows devs wanted.
            // 0xBB GG RR XX in memory
            u8 *alpha = (u8 *)pixel;
        }

        row += game_back_buffer->pitch;
    }
}


void game_draw_bitmap(Game_Back_Buffer *game_back_buffer, Bitmap *bitmap, f32 x_pos, f32 y_pos) {
    s32 min_x = round_f32_to_s32(x_pos);
    s32 min_y = round_f32_to_s32(y_pos);
    s32 max_x = round_f32_to_s32(x_pos + (f32)bitmap->width);
    s32 max_y = round_f32_to_s32(y_pos + (f32)bitmap->height);
    
    if (min_x < 0) { min_x = 0; }

    if (min_y < 0) { min_y = 0; }

    if (max_x > game_back_buffer->width)  { max_x = game_back_buffer->width;  }

    if (max_y > game_back_buffer->height) { max_y = game_back_buffer->height; }

    x_pos += X_OFFSET;
    y_pos += Y_OFFSET;

    u32 *src_row   = (u32 *)bitmap->data + bitmap->width * (bitmap->height - 1);
    u8  *dest_row  = ((u8 *)game_back_buffer->memory +
                     min_x*game_back_buffer->bpp     +
                     min_y*game_back_buffer->pitch);
    
    for (s32 y = min_y; y < max_y; ++y) {
        u32 *dest = (u32 *)dest_row;
        u32 *src  = src_row;
        for (s32 x = min_x; x < max_x; ++x) {

/*
  Alpha Test:            
            As opposed to alpha test i.e. clapping on an alpha
            of 128.
            if (alpha > 120) { 
                draw pixel
            }

  Linear Blend:
            c = A + t(B - A);
            c = A + tb - tA;
            c = A - tA + tb;
            c = (1-t)A + tb = Linear blend;
            
*/
#if 0
            // Alpha test (this is working).

            if ((*src >> 24) > 128) {
                *dest = *src;
            }
#endif 

#if 0 
            // Linear (conventional alpha blending).
            // blend(source, dest) = (source.rgb * source.a) + (dest.rgb * (1 - source.a))

            f32 alpha     = (f32)((*src >> 24) & 0xFF) / 255.0f;
            f32 src_red   = (f32)((*src >> 16) & 0xFF);
            f32 src_green = (f32)((*src >>  8) & 0xFF);
            f32 src_blue  = (f32)((*src >>  0) & 0xFF);
            
            f32 dest_red   = (f32)((*dest >> 16) & 0xFF);
            f32 dest_green = (f32)((*dest >>  8) & 0xFF);
            f32 dest_blue  = (f32)((*dest >>  0) & 0xFF);

            f32 red   = (1.0f-alpha)*dest_red   + alpha*src_red;
            f32 green = (1.0f-alpha)*dest_green + alpha*src_green;
            f32 blue  = (1.0f-alpha)*dest_blue  + alpha*src_blue;

            *dest = (((u32)(red   + 0.5f) << 16) |
                     ((u32)(green + 0.5f) <<  8) | 
                     ((u32)(blue  + 0.5f) <<  0));

#endif

#if 1 
            // Premultiplied Alpha blending
            // blend(source, dest) = source.rgb + (dest.rgb * (1 - source.a))
            f32 alpha     = (f32)((*src >> 24) & 0xFF) / 255.0f;
            f32 src_red   = (f32)((*src >> 16) & 0xFF);
            f32 src_green = (f32)((*src >>  8) & 0xFF);
            f32 src_blue  = (f32)((*src >>  0) & 0xFF);
            
            // Convert non-premultiplied color into premultiplied format.
            //color.rgb *= color.a
            src_red   *= alpha;
            src_green *= alpha;
            src_blue  *= alpha;
            
            f32 dest_red   = (f32)((*dest >> 16) & 0xFF);
            f32 dest_green = (f32)((*dest >>  8) & 0xFF);
            f32 dest_blue  = (f32)((*dest >>  0) & 0xFF);
            
            f32 red   = (1.0f-alpha)*dest_red   + src_red;
            f32 green = (1.0f-alpha)*dest_green + src_green;
            f32 blue  = (1.0f-alpha)*dest_blue  + src_blue;

            *dest = (((u32)(red   + 0.5f) << 16) |
                     ((u32)(green + 0.5f) <<  8) | 
                     ((u32)(blue  + 0.5f) <<  0));
#endif

            ++dest;
            ++src;
        }

        dest_row += game_back_buffer->pitch;
        src_row  -= bitmap->width;
    }
}

void game_update_and_render(Game_Memory *game_memory, Game_Back_Buffer *game_back_buffer) {
    Game_State *game_state = (Game_State *)game_memory->persistent_memory;
    if (!game_memory->valid) {
        game_state->tone_per_second = 256;
        game_state->green_offset    = 0;
        game_state->blue_offset     = 0;
    }

    draw_gradient(game_back_buffer, game_state->blue_offset, game_state->green_offset);
}

void game_get_sound_samples(Game_Memory *game_memory, Sound_Manager *sound_manager) { 
    //update_sound(sound_manager);
}

