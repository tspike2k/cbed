//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
TODO:
    We should be able to do automatic controller mapping using SDLs massive gamepad control map. Events automatically use the mapping.
*/

#include "common.h"

typedef enum{
    Gamepad_Button_Unknown,

    Gamepad_Button_Left,
    Gamepad_Button_Right,
    Gamepad_Button_Up,
    Gamepad_Button_Down,
    Gamepad_Button_A,
    Gamepad_Button_B,
    Gamepad_Button_X,
    Gamepad_Button_Y,
    Gamepad_Button_L1,
    Gamepad_Button_R1,

    Gamepad_Button_Max,
} Gamepad_Button;

typedef enum{
    Gamepad_Stick_Unknown,

    Gamepad_Stick_LX,
    Gamepad_Stick_LY,
    Gamepad_Stick_RX,
    Gamepad_Stick_RY,

    Gamepad_Stick_Max,
} Gamepad_Stick;

typedef enum {
    Gamepad_Event_Unknown,
    Gamepad_Event_Button,
    Gamepad_Event_Stick,
    Gamepad_Event_Connect,
    Gamepad_Event_Disconnect,
} Gamepad_Event_Type;

typedef struct {
    Gamepad_Event_Type type;
    u32 id;
    f32 value;
} Gamepad_Event;

Ceabed_API bool   gamepad_begin(const char *bindings_file_path, Buffer *temp);
Ceabed_API void   gamepad_end();
Ceabed_API void   gamepad_update(Buffer *temp);
Ceabed_API bool   gamepad_poll(u32 gamepad_index, Gamepad_Event *event);
Ceabed_API u32    gamepad_get_count();
Ceabed_API String gamepad_get_input_event_string(Gamepad_Event evt);
