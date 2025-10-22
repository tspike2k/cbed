//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_DISPLAY_H
#define CEABED_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

enum{
    Display_Flag_HW_Rendering = (1 << 0),
};

enum{
    Key_Modifier_Ctrl = (1 << 0),
};

enum{
    Key_ID_A,
    Key_ID_B,
    Key_ID_C,
    Key_ID_D,
    Key_ID_E,
    Key_ID_F,
    Key_ID_G,
    Key_ID_H,
    Key_ID_I,
    Key_ID_J,
    Key_ID_K,
    Key_ID_L,
    Key_ID_M,
    Key_ID_N,
    Key_ID_O,
    Key_ID_P,
    Key_ID_Q,
    Key_ID_R,
    Key_ID_S,
    Key_ID_T,
    Key_ID_U,
    Key_ID_V,
    Key_ID_W,
    Key_ID_X,
    Key_ID_Y,
    Key_ID_Z,

    Key_ID_0,
    Key_ID_1,
    Key_ID_2,
    Key_ID_3,
    Key_ID_4,
    Key_ID_5,
    Key_ID_6,
    Key_ID_7,
    Key_ID_8,
    Key_ID_9,

    Key_ID_F1,
    Key_ID_F2,
    Key_ID_F3,
    Key_ID_F4,
    Key_ID_F5,
    Key_ID_F6,
    Key_ID_F7,
    Key_ID_F8,
    Key_ID_F9,
    Key_ID_F10,
    Key_ID_F11,
    Key_ID_F12,

    Key_ID_Arrow_Up,
    Key_ID_Arrow_Down,
    Key_ID_Arrow_Left,
    Key_ID_Arrow_Right,
    Key_ID_Enter,
    Key_ID_Escape,
    Key_ID_Delete,
    Key_ID_Backspace,
};

enum {
    Event_Type_None,
    Event_Type_Window_Close,
    Event_Type_Key,
    Event_Type_Mouse_Motion,
    Event_Type_Button,
    Event_Type_Paste,
    Event_Type_Text,
};

enum{
    Button_ID_None,
    Button_ID_Mouse_Left,
    Button_ID_Mouse_Right,
    Button_ID_Mouse_Middle,
};

typedef struct{
    uint32_t type;
    uint32_t id;
    uint32_t modifier;
    bool     pressed; // TODO: Make these flags?
    bool     is_repeat;
} Event_Key;

typedef struct{
    uint32_t type;
    // TODO: Have a mouse id?
    int   pixel_x;
    int   pixel_y;
    float rel_x;
    float rel_y;
} Event_Mouse_Motion;

typedef struct{
    uint32_t type;
    uint32_t id;
    bool     pressed; // TODO: Button status flag?
} Event_Button;

enum{
    Clipboard_Type_Unknown,
    Clipboard_Type_Text,
};

typedef struct{
    uint32_t type;
    uint32_t paste_type;
    void    *data;
    size_t   data_size;
} Event_Paste;

typedef struct{
    uint32_t type;
    char    *data; // TODO: Should these be utf-32 codepoints instead?
    size_t   data_count;
} Event_Text;

typedef struct {
    bool consumed;
    union{
        uint32_t           type;
        Event_Key          key;
        Event_Mouse_Motion mouse_motion;
        Event_Button       button;
        Event_Paste        paste;
        Event_Text         text;
    };
} Event;

typedef struct{
    uint32_t  width;
    uint32_t  height;
    uint32_t *pixels;
} Display_Backbuffer;

typedef struct{
    uint32_t window_flags;
    uint32_t window_width;
    uint32_t window_height;
} Display_Info;

bool display_begin(const char *window_title, uint32_t width, uint32_t height, uint32_t window_flags);
void display_end();
Display_Backbuffer display_get_sw_backbuffer();
void display_flip_backbuffer();
Display_Info display_get_info();

#endif // CEABED_DISPLAY_H
