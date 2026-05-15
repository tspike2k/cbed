//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
TODO:
    - Parse and apply SDL gamepad bindings and deadzones.
    - Have functions for configuring bindings and deadzones and saving/loading those preferences.
    - Handle adding/removing gamepads.

    When doing mappings, things might turn out to be a little complicated. Some things, like
    shoulder buttons, are often an axis. That means that even buttons might require a deadzone
    value. But it also means we need to be able bind controller axis to buttons. On my controller,
    the d-pad is modeled as two seperate axis rather than four buttons. We should allow the user
    to bind those axis to different buttons. Which means, axis bindings for buttons need to know
    the sign of the axis.

    It's also important to note that some axis are actually digital, with values ranging from
    0 to 1. How we can tell the difference, I'm not sure. Perhaps on Linux this is well-defined.
*/

#include "common.h"

typedef enum{
    Gamepad_Input_Unknown,

    Gamepad_Button_Left,
    Gamepad_Button_Right,
    Gamepad_Button_Up,
    Gamepad_Button_Down,
    Gamepad_Button_A,
    Gamepad_Button_B,
    Gamepad_Button_X,
    Gamepad_Button_Y,
    Gamepad_Button_L1,
    Gamepad_Button_L2,
    Gamepad_Button_L3,
    Gamepad_Button_R1,
    Gamepad_Button_R2,
    Gamepad_Button_R3,
    Gamepad_Button_Start,
    Gamepad_Button_Select,

    Gamepad_Axis_LX,
    Gamepad_Axis_LY,
    Gamepad_Axis_RX,
    Gamepad_Axis_RY,
    Gamepad_Axis_Dir_X,
    Gamepad_Axis_Dir_Y,

    Gamepad_Input_Max,
} Gamepad_Input;

typedef enum {
    Gamepad_Event_Unknown,
    Gamepad_Event_Input,
    Gamepad_Event_Connect,
    Gamepad_Event_Disconnect,
} Gamepad_Event_Type;

typedef struct {
    Gamepad_Event_Type type;
    u32 id;
    f32 value;
    u32 internal_type;
    u32 internal_id;
} Gamepad_Event;

Ceabed_API bool   gamepad_begin(const char *bindings_file_path, Buffer *temp);
Ceabed_API void   gamepad_end();
Ceabed_API void   gamepad_update(Buffer *temp);
Ceabed_API bool   gamepad_next_event(u32 gamepad_index, Gamepad_Event *event);
Ceabed_API bool   gamepad_is_connected(u32 gamepad_index);
Ceabed_API u32    gamepad_get_count();
Ceabed_API String gamepad_get_event_string(Gamepad_Event event, Buffer *buffer);
