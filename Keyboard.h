#pragma once
#include "Types.h"

// @Note: Rename this file to User_Input.h

const u16 VK_CODE_LEN = 256;

struct Keyboard_Button {
    s32  half_transition_count;
    bool is_down;

    f32 min_x;
    f32 min_y;
    f32 max_x;
    f32 max_y;

    f32 start_x;
    f32 start_y;
    f32 end_x;
    f32 end_y;
};

struct Keyboard {

    union { 
        Keyboard_Button keyboard_buttons[VK_CODE_LEN];

        // Create an enum for this so we can just index the keyboard buttons.
/*
        struct {
            Keyboard_Button up;
            Keyboard_Button down;
            Keyboard_Button left;
            Keyboard_Button right;

            Keyboard_Button a;
            Keyboard_Button b;
            Keyboard_Button c;
            Keyboard_Button d;
            Keyboard_Button e;
            Keyboard_Button f;
            Keyboard_Button g;
            Keyboard_Button h;
            Keyboard_Button i;
            Keyboard_Button j;
            Keyboard_Button k;
            Keyboard_Button l;
            Keyboard_Button m;
            Keyboard_Button n;
            Keyboard_Button o;
            Keyboard_Button p;
            Keyboard_Button q;
            Keyboard_Button r;
            Keyboard_Button s;
            Keyboard_Button t;
            Keyboard_Button u;
            Keyboard_Button v;
            Keyboard_Button w;
            Keyboard_Button x;
            Keyboard_Button y;
            Keyboard_Button z;
            
            Keyboard_Button num_0;
            Keyboard_Button num_1;
            Keyboard_Button num_2;
            Keyboard_Button num_3;
            Keyboard_Button num_4;
            Keyboard_Button num_5;
            Keyboard_Button num_6;
            Keyboard_Button num_7;
            Keyboard_Button num_8;
            Keyboard_Button num_9;
        };
*/
    };
};


struct Mouse_Button { 
    bool is_down;
};

struct Mouse { 
    s32 x_offset;
    s32 y_offset;
    
    Mouse_Button left;
    Mouse_Button right;
    
    Mouse_Button middle;
};
