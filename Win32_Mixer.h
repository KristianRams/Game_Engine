#pragma once

#include <dsound.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <windows.h>
#include "Wav_File.h"
#include "String.h"
#include "Array.h"

#include <xmmintrin.h>

#define SAMPLES_PER_SECOND 24000

// Direct Sound Crap...
enum Cooperative_Level : u8 {
    _DSSCL_NORMAL       = 0x01,
    _DSSCL_PRIORITY     = 0x02,
    _DSSCL_EXCLUSIVE    = 0x03,
    _DSSCL_WRITEPRIMARY = 0x04,
};

enum Speaker_Geometry : u8 {
    _DSSPEAKER_GEOMETRY_MIN    =   5,
    _DSSPEAKER_GEOMETRY_NARROW =  10,
    _DSSPEAKER_GEOMETRY_WIDE   =  20,
    _DSSPEAKER_GEOMETRY_MAX    = 180,
};

enum Speaker_Configurations : u8 {
    _DSSPEAKER_DIRECTOUT        = 0x00,
    _DSSPEAKER_HEADPHONE        = 0x01,
    _DSSPEAKER_MONO             = 0x02,
    _DSSPEAKER_QUAD             = 0x03,
    _DSSPEAKER_STEREO           = 0x04,
    _DSSPEAKER_SURROUND         = 0x05,
    _DSSPEAKER_5POINT1_BACK     = 0x06,
    _DSSPEAKER_7POINT1_WIDE     = 0x07,
    _DSSPEAKER_7POINT1_SURROUND = 0x08,
    _DSSPEAKER_5POINT1_SURROUND = 0x09,
};

enum Speaker_Position : u32 {
    _SPEAKER_FRONT_LEFT            = 0x001,
    _SPEAKER_FRONT_RIGHT           = 0x002,
    _SPEAKER_FRONT_CENTER          = 0x004,
    _SPEAKER_LOW_FREQUENCY         = 0x008,
    _SPEAKER_BACK_LEFT             = 0x010,
    _SPEAKER_BACK_RIGHT            = 0x020,
    _SPEAKER_FRONT_LEFT_OF_CENTER  = 0x040,
    _SPEAKER_FRONT_RIGHT_OF_CENTER = 0x080,
    _SPEAKER_BACK_CENTER           = 0x100,
    _SPEAKER_SIDE_LEFT             = 0x200,
    _SPEAKER_SIDE_RIGHT            = 0x400,

    _SPEAKER_TOP_CENTER            = 0x00800,
    _SPEAKER_TOP_FRONT_LEFT        = 0x01000,
    _SPEAKER_TOP_FRONT_CENTER      = 0x02000,
    _SPEAKER_TOP_FRONT_RIGHT       = 0x04000,
    _SPEAKER_TOP_BACK_LEFT         = 0x08000,
    _SPEAKER_TOP_BACK_CENTER       = 0x10000,
    _SPEAKER_TOP_BACK_RIGHT        = 0x20000,
};

enum Direct_Sound_Buffer_Capability : u32 {
    _DSBCAPS_PRIMARYBUFFER        = 0x00001,
    _DSBCAPS_STATIC               = 0x00002,
    _DSBCAPS_LOCHARDWARE          = 0x00004,
    _DSBCAPS_LOCSOFTWARE          = 0x00008,
    _DSBCAPS_CTRL3D               = 0x00010,
    _DSBCAPS_CTRLFREQUENCY        = 0x00020,
    _DSBCAPS_CTRLPAN              = 0x00040,
    _DSBCAPS_CTRLVOLUME           = 0x00080,
    _DSBCAPS_CTRLPOSITIONNOTIFY   = 0x00100,
    _DSBCAPS_CTRLFX               = 0x00200,
    _DSBCAPS_STICKYFOCUS          = 0x04000,
    _DSBCAPS_GLOBALFOCUS          = 0x08000,
    _DSBCAPS_GETCURRENTPOSITION2  = 0x10000,
    _DSBCAPS_MUTE3DATMAXDISTANCE  = 0x20000,
    _DSBCAPS_LOCDEFER             = 0x40000,
    _DSBCAPS_TRUEPLAYPOSITION     = 0x80000,
};

u8 _DSSPEAKER_CONFIG(s32 speaker_config) {
    return (u8)(speaker_config & 0xff);
}

u8 _DSSPEAKER_GEOMETRY (s32 speaker_config) {
    return (u8)((speaker_config >> 16) & 0xff);
}

struct Fill_Region {
    VOID *region1;
    DWORD region1_size;
    VOID *region2;
    DWORD region2_size;
};

enum Sound_Flags {
    REPEATING   = 0x01,
    SPATIALIZED = 0x02,
    IS_MUSIC    = 0x04,
    FADING_OUT  = 0x08,
};

struct Sound_Manager {
    bool playing;

    s32 safety_bytes;

    // left channel  = s16
    // right channel = s16
    // total sample  = s32 = 4 bytes
    const u8 bytes_per_sample = sizeof(s16) * 2;
    
    // How many seconds to make the secondary buffer.
    s32 seconds;

    s32 samples_per_second;

    s16 num_channels;
    Array <string> channel_names;

    Sound_Flags flags;

    // DirectSound write and play cursor
    s64 write_cursor;
    s64 play_cursor; // We don't really need this.

    IDirectSound8       *direct_sound; // Interface pointer.
    IDirectSoundBuffer8 *primary_buffer;
    IDirectSoundBuffer8 *secondary_buffer;

    // In bytes.
    s64 secondary_buffer_size;

    // Cached loaded sounds.
    Array <Sound_Data *> loaded_sounds;

    // Linked list of playing sounds.
    Playing_Sound *playing_sounds;
    
    s32 running_sample_index;
};

void init_wave_format(WAVEFORMATEX *wave_format, s16 num_channels, u32 samples_per_second) {
    wave_format->wFormatTag      = WAVE_FORMAT_EXTENSIBLE;//WAVE_FORMAT_PCM;
    wave_format->nChannels       = num_channels;
    wave_format->nSamplesPerSec  = samples_per_second;
    wave_format->wBitsPerSample  = 16;
    wave_format->nBlockAlign     = (wave_format->wBitsPerSample / 8) * wave_format->nChannels;
    wave_format->nAvgBytesPerSec =  wave_format->nSamplesPerSec * wave_format->nBlockAlign;
    wave_format->cbSize          =  22; // Needs to be 22 per windows specifications.
}

// Silent buffer
void clear_secondary_buffer(Sound_Manager *sound_manager) {
    Fill_Region fill_region;

    sound_manager->secondary_buffer->Lock(0, sound_manager->secondary_buffer_size,
                                          &fill_region.region1, &fill_region.region1_size,
                                          &fill_region.region2, &fill_region.region2_size,
                                          0);

    // Clear the contents of the sound buffer as Direct Sound doesn't zero init this stuff.
    memset((void *)fill_region.region1, 0x0, fill_region.region1_size);
    memset((void *)fill_region.region2, 0x0, fill_region.region2_size);

    sound_manager->secondary_buffer->Unlock(fill_region.region1, fill_region.region1_size,
                                            fill_region.region2, fill_region.region2_size);
}

bool init_sound_manager(Sound_Manager *sound_manager, HWND window_handle) {
    HRESULT hr;

    hr = DirectSoundCreate8(null, &(sound_manager->direct_sound), null);
    if (!SUCCEEDED(hr)) { return false; }

    hr = sound_manager->direct_sound->SetCooperativeLevel(window_handle, _DSSCL_PRIORITY);
    if (!SUCCEEDED(hr)) { return false; }

    DWORD speaker_config = 0;
    hr = sound_manager->direct_sound->GetSpeakerConfig(&speaker_config);
    if (!SUCCEEDED(hr)) { return false; }

    u8 config   = _DSSPEAKER_CONFIG(speaker_config);
    u8 geometry = _DSSPEAKER_GEOMETRY(speaker_config);

    u32 speaker_position = 0;

    switch (config) {
        case _DSSPEAKER_5POINT1_BACK:
        case _DSSPEAKER_5POINT1_SURROUND: {
            array_add(&sound_manager->channel_names, make_literal("Front Left"));
            array_add(&sound_manager->channel_names, make_literal("Front Right"));
            array_add(&sound_manager->channel_names, make_literal("Center"));
            array_add(&sound_manager->channel_names, make_literal("Subwoofer"));
            array_add(&sound_manager->channel_names, make_literal("Back Left"));
            array_add(&sound_manager->channel_names, make_literal("Back Right"));
            speaker_position = (_SPEAKER_FRONT_LEFT | _SPEAKER_FRONT_RIGHT | _SPEAKER_FRONT_CENTER | _SPEAKER_LOW_FREQUENCY |
                                _SPEAKER_SIDE_LEFT  | _SPEAKER_SIDE_RIGHT);
            break;
        }
        case _DSSPEAKER_7POINT1_WIDE:
        case _DSSPEAKER_7POINT1_SURROUND: {
            array_add(&sound_manager->channel_names, make_literal("Front Left"));
            array_add(&sound_manager->channel_names, make_literal("Front Right"));
            array_add(&sound_manager->channel_names, make_literal("Center"));
            array_add(&sound_manager->channel_names, make_literal("Subwoofer"));
            array_add(&sound_manager->channel_names, make_literal("Back Left"));
            array_add(&sound_manager->channel_names, make_literal("Back Right"));
            array_add(&sound_manager->channel_names, make_literal("Extend Left"));
            array_add(&sound_manager->channel_names, make_literal("Extend Right"));
            speaker_position = (_SPEAKER_FRONT_LEFT | _SPEAKER_FRONT_RIGHT | _SPEAKER_FRONT_CENTER | _SPEAKER_LOW_FREQUENCY |
                                _SPEAKER_SIDE_LEFT  | _SPEAKER_SIDE_RIGHT  | _SPEAKER_BACK_LEFT    | _SPEAKER_BACK_RIGHT);
            break;
        }
        default: {
            array_add(&sound_manager->channel_names, make_literal("Left"));
            array_add(&sound_manager->channel_names, make_literal("Right"));
            speaker_position = (_SPEAKER_FRONT_LEFT | _SPEAKER_FRONT_RIGHT);
            break;
        }
    }

    sound_manager->num_channels = sound_manager->channel_names.size;

    DSBUFFERDESC primary_buffer_description    = {};
    primary_buffer_description.dwSize          = sizeof(DSBUFFERDESC);
    primary_buffer_description.dwFlags         = _DSBCAPS_PRIMARYBUFFER;
    primary_buffer_description.guid3DAlgorithm = GUID_NULL;

    hr = sound_manager->direct_sound->CreateSoundBuffer(&primary_buffer_description, (IDirectSoundBuffer **)&(sound_manager->primary_buffer), null);
    if (!SUCCEEDED(hr)) { return false; }

    // For more than 2 channels we have to use WAVEFORMATEXTENSIBLE.
    WAVEFORMATEXTENSIBLE wave_format_ex = {};
    wave_format_ex.dwChannelMask        = speaker_position;
    wave_format_ex.SubFormat            = KSDATAFORMAT_SUBTYPE_PCM;
    
    sound_manager->seconds = 1;
    sound_manager->samples_per_second    = SAMPLES_PER_SECOND * sound_manager->num_channels;

    // Create a 1 second buffer.
    sound_manager->secondary_buffer_size = sound_manager->samples_per_second * sound_manager->bytes_per_sample * sound_manager->seconds;

    init_wave_format(&wave_format_ex.Format, sound_manager->num_channels, sound_manager->samples_per_second);

    hr = sound_manager->primary_buffer->SetFormat(&wave_format_ex.Format);

    if (!SUCCEEDED(hr)) { return false; }

    DSBUFFERDESC secondary_buffer_description  = {};
    secondary_buffer_description.dwSize        = sizeof(DSBUFFERDESC);
    secondary_buffer_description.lpwfxFormat   = &wave_format_ex.Format;
    secondary_buffer_description.dwFlags       = _DSBCAPS_CTRLVOLUME | _DSBCAPS_GETCURRENTPOSITION2;
    secondary_buffer_description.dwBufferBytes = sound_manager->secondary_buffer_size;

    // Create secondary sound buffer.
    hr = sound_manager->direct_sound->CreateSoundBuffer(&secondary_buffer_description, (IDirectSoundBuffer **)(&sound_manager->secondary_buffer), null);
    if (!SUCCEEDED(hr)) { return false; }

    clear_secondary_buffer(sound_manager);

    sound_manager->secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);

    // Even though we are playing we should not be outputting anything.
    // Because of this set playing to be false.
    sound_manager->playing = false;
    
    sound_manager->running_sample_index = 0;
    
    sound_manager->playing_sounds = null;

    return true;
}

void deinit_sound_manager(Sound_Manager *sound_manager) {
    if (!sound_manager) { return; }

    if (sound_manager->playing) { sound_manager->secondary_buffer->Stop(); }

    if (sound_manager->primary_buffer)   { sound_manager->primary_buffer  ->Release(); }
    if (sound_manager->secondary_buffer) { sound_manager->secondary_buffer->Release(); }
    if (sound_manager->direct_sound)     { sound_manager->direct_sound    ->Release(); }

    sound_manager->playing = false;

    for (int i = 0; i < sound_manager->loaded_sounds.size; ++i) {
        delete sound_manager->loaded_sounds[i]->raw_file;
    }
    
    array_deinit(&sound_manager->channel_names);

    array_deinit(&sound_manager->loaded_sounds);
}

Sound_Data *find_and_load_sound(Sound_Manager *sound_manager, char *file_name) {
    // All of our sound assets should be located in the data test3 directory.
    string aa = "./data/test3/";
    aa += file_name;
    char *backup_path      = (char *)aa.data;
    Sound_Data *sound_data = load_wav_file(backup_path, file_name);

    if (!sound_data) {
        output_debug_string("Failed to find file when loading sound data");
        return NULL;
    }

    array_add(&sound_manager->loaded_sounds, sound_data);
    return sound_data;
}

#define Align8(value) (((value) + 7) &  ~7)

Playing_Sound *play_sound(Sound_Manager *sound_manager, char *file_name) {
    Sound_Data *sound_data = find_and_load_sound(sound_manager, file_name);
    if (!sound_data) { return null; }
    
    Playing_Sound *playing_sound = new Playing_Sound;
    if (!sound_manager->playing_sounds) {
        playing_sound->name      = file_name;
        playing_sound->volume[0] = 1.0f;
        playing_sound->volume[1] = 1.0f;
        playing_sound->next      = null;
        playing_sound->current_sample_position = 0;
        playing_sound->sound_data = sound_data;
        sound_manager->playing_sounds = playing_sound;
    } else { 
        for (Playing_Sound *current_playing_sound = sound_manager->playing_sounds; current_playing_sound;) { 
            Playing_Sound *next_playing_sound = current_playing_sound->next;
            if (!next_playing_sound) { 
                playing_sound->name      = file_name;
                playing_sound->volume[0] = 1.0f;
                playing_sound->volume[1] = 1.0f;
                playing_sound->next      = null;
                playing_sound->current_sample_position = 0;
                playing_sound->sound_data = sound_data;
                current_playing_sound->next = playing_sound;
            }
            current_playing_sound = next_playing_sound;
        }
    }
    return playing_sound;
}


void output_sound(Sound_Manager *sound_manager) {
    if (!sound_manager) { return; }

    DWORD bytes_to_write;
    bool found = false;

    // (48000 samples/per 1 second)  / (1 second / 30 frames) = 1600 samples per frame
    if (SUCCEEDED(sound_manager->secondary_buffer->GetCurrentPosition((DWORD *)&sound_manager->play_cursor, (DWORD *)&sound_manager->write_cursor))) {
        // If this isn't the first time that we are playing this sound then we need to set the
        // write cursor to the current sample position in the audio file that we left off on.
        if (sound_manager->running_sample_index != 0) {
            sound_manager->write_cursor = (sound_manager->running_sample_index * sound_manager->bytes_per_sample) % sound_manager->secondary_buffer_size;
        } else { 
            sound_manager->running_sample_index = sound_manager->write_cursor / sound_manager->bytes_per_sample;
        }

        if (sound_manager->write_cursor > sound_manager->play_cursor) {
            bytes_to_write = sound_manager->secondary_buffer_size - sound_manager->write_cursor;
            bytes_to_write += sound_manager->play_cursor;
        } else if (sound_manager->write_cursor < sound_manager->play_cursor) {
            bytes_to_write = sound_manager->play_cursor - sound_manager->write_cursor;
        } else {
            bytes_to_write = sound_manager->secondary_buffer_size - sound_manager->write_cursor;
            // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee418062(v=vs.85)
            // "The write cursor is the point in the buffer ahead of which it is safe to write data to the buffer.
            // Data should not be written to the part of the buffer after the play cursor and before the write cursor.".
        }
    } else {
        // If we're not able to query the sound card for the current write and play cursor then return.
        output_debug_string("Failed to query the sound card for the read and write cursors");
        return;
    }
    
    
    s32 total_sample_count = Align8(bytes_to_write / sound_manager->bytes_per_sample);

    f32 *channel0 = new f32[total_sample_count]();
    f32 *channel1 = new f32[total_sample_count]();

    __m128 Zero = _mm_set1_ps(0.0f);

    Sound_Data *sound_data;

    for (Playing_Sound *playing_sound = sound_manager->playing_sounds; playing_sound;) { 
        Playing_Sound *next_playing_sound = playing_sound->next;
        
        f32 left_channel_volume  = playing_sound->volume[0];
        f32 right_channel_volume = playing_sound->volume[1];
        f32 *dest0 = channel0;
        f32 *dest1 = channel1;
        
        s32 samples_to_mix = total_sample_count;
        sound_data = playing_sound->sound_data;
        if (!sound_data) { 
            output_debug_string("Failed to get sound data.");
            return;
        }
        s32 samples_remaining = sound_data->sample_count - playing_sound->current_sample_position;
        if (samples_to_mix > samples_remaining) { samples_to_mix = samples_remaining; }
        
        // @Todo: There is a bug here when converting from s16 samples to float when mixing.
        for (s32 sample_index = playing_sound->current_sample_position; 
             sample_index < (playing_sound->current_sample_position + samples_to_mix);
             ++sample_index) { 
            f32 sample_value = sound_data->samples[0][sample_index];
            *dest0++ += (left_channel_volume  * sample_value);
            *dest1++ += (right_channel_volume * sample_value);
        }

        playing_sound->current_sample_position += samples_to_mix;

        // If the sound flags are marked looping then reset the current_sample_position to 0.
        if (playing_sound->current_sample_position == sound_data->sample_count) {
            //playing_sound->current_sample_position = 0;
        }

        playing_sound = next_playing_sound;
    }


    f32 *source0 = channel0;
    f32 *source1 = channel1;

    Fill_Region fill_region;
    sound_manager->secondary_buffer->Lock(sound_manager->write_cursor, bytes_to_write,
                                          &fill_region.region1, &fill_region.region1_size,
                                          &fill_region.region2, &fill_region.region2_size,
                                          0);
    

    s32 region1_sample_count = fill_region.region1_size / sound_manager->bytes_per_sample;
    s16 *dest_sample = (s16 *)fill_region.region1;
    for (DWORD sample_index = 0; sample_index < region1_sample_count; ++sample_index) {
        *dest_sample++ = (s16)(*source0++ + 0.5f);
        *dest_sample++ = (s16)(*source1++ + 0.5f);
        ++sound_manager->running_sample_index;
    }


    s32 region2_sample_count = fill_region.region2_size / sound_manager->bytes_per_sample;
    dest_sample = (s16 *)fill_region.region2;
    for (DWORD sample_index = 0; sample_index < region2_sample_count; ++sample_index) {
        *dest_sample++ = (s16)(*source0++ + 0.5f);
        *dest_sample++ = (s16)(*source1++ + 0.5f);
        ++sound_manager->running_sample_index;
    }
    
    assert((region1_sample_count + region2_sample_count) == total_sample_count);

    sound_manager->secondary_buffer->Unlock(fill_region.region1, fill_region.region1_size, fill_region.region2, fill_region.region2_size);

    // @Cleanup: Only set this once when we are playing sound for the 
    // first time.
    sound_manager->playing = true;

    // @Todo: Use arena.
    delete[] channel0;
    delete[] channel1;
}
