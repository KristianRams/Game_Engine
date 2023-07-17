#pragma once

#include "Types.h"


#define CHUNK_ID(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

enum {
    RIFF_ID = CHUNK_ID('R', 'I', 'F', 'F'),
    WAVE_ID = CHUNK_ID('W', 'A', 'V', 'E'),
    FMT_ID  = CHUNK_ID('f', 'm', 't', ' '),
    DATA_ID = CHUNK_ID('d', 'a', 't', 'a'),
};

#pragma pack(push, 1)
struct Wave_Format_Ex {
    s16  wFormatTag;
    s16  nChannels;
    s32  nSamplesPerSec;
    s32  nAvgBytesPerSec;
    s16  nBlockAlign;
    s16  wBitsPerSample;
};
#pragma pack(pop)

enum Sound_Type_Flags { 
    OGG   = 0x01,
    ADPCM = 0x02,
};

struct Sound_Data { 
    // This should be freed since it was 
    // allocated on the heap.
    u8 *raw_file;

    char *name;

    Wave_Format_Ex format;
    
    u32 sample_count;
    s16 *samples[2];
    
    // The current sample position you are in the samples buffer.
    // This is signed that we can delay the sound by some time t.
    
    Sound_Type_Flags flags;
};


struct Playing_Sound {
    f32 volume[2];
    char *name;

    // Just keep a pointer to the sound_data associated with this sound.
    // We'll just access it to read the samples.
    Sound_Data *sound_data;
    Playing_Sound *next;
    // Signed in case we want to deplay sound by making 
    // current_sample_position negative.
    s32 current_sample_position;
};

Sound_Data *load_wav_file(char *wav_file_full_path, char *wav_file_name);

