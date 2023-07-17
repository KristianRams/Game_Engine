#include "Wav_File.h"

void advance(u8 **cursor, s64 bytes) { 
    *cursor += bytes;
}

bool chunk_id_matches(u8 **cursor, char *id) {
    bool matches = (memcmp(*cursor, id, 4) == 0);
    if (matches) { return true; }
    return false; 
}

Sound_Data *parse_wav_file(u8 *data, u64 size) {
    Sound_Data *sound_data = new Sound_Data;

    sound_data->raw_file = data;

    u8 *cursor = data;
    
    bool success = chunk_id_matches(&cursor, "RIFF");
    if (!success) { return false; }

    advance(&cursor, 4);
    
    u32 riff_chunk_size = *(u32 *)cursor;
    
    advance(&cursor, 4);
    
    success = chunk_id_matches(&cursor, "WAVE");
    if (!success) { return NULL; }

    advance(&cursor, 4);

    // We don't care about anything other than fmt and data.
    success = chunk_id_matches(&cursor, "fmt ");
    if (!success) { return NULL; }

    advance(&cursor, 4);
    
    u32 fmt_chunk_size = *(u32 *)cursor;
    
    advance(&cursor, 4);
    
    memcpy(&sound_data->format, cursor, sizeof(Wave_Format_Ex));
    
    advance(&cursor, sizeof(Wave_Format_Ex));
    
    success = chunk_id_matches(&cursor, "data");
    
    if (!success) { return NULL; }
    
    advance(&cursor, 4);
    
    u32 data_chunk_size = *(u32 *)cursor;
    
    advance(&cursor, 4);

    // Since this interleaved we need to deinterleave.
    //sound_data->samples      = (s16 *)cursor;
    sound_data->sample_count = data_chunk_size / (sound_data->format.nChannels * sizeof(s16)); 

    assert(sound_data->format.nChannels == 2);

    sound_data->samples[0] = (s16 *)cursor;

    
    sound_data->samples[1] =(s16 *)cursor + sound_data->sample_count;

    // De-interleave the channels so that it's all left in samples[0]
    // and all right in samples[1].
    // At this point the right channel is all garbage.
    for (u32 sample_index = 0; sample_index < sound_data->sample_count; ++sample_index ) { 
        s16 source = sound_data->samples[0][2 * sample_index];
        sound_data->samples[0][2 * sample_index] = sound_data->samples[0][sample_index];
        sound_data->samples[0][sample_index] = source;
    }

    
    return sound_data;
}

// path_to_file is the full path
// name_of_file is just the name without the .wav extension.
Sound_Data *load_wav_file(char *wav_file_full_path, char *wav_file_name) {
    u8 *data;
    
    s64 size = read_file(wav_file_full_path, (void **)&data);
    
    Sound_Data *sound_data = parse_wav_file(data, size);
    sound_data->name = wav_file_name;
    
    return sound_data;
}
