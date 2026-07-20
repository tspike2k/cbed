//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "common.c"
#include "display.h"
#include "opengl.c"
#include "math.c"
#include "font.c"
#include "draw.c"
#include "gamepad.c"

u8 g_memory[4*1024*1024];

f32 g_gamepad_status[Gamepad_Input_Max];

static bool is_stick(Gamepad_Input input){
    bool result = false;
    switch(input){
        default: break;

        case Gamepad_Axis_LX:
        case Gamepad_Axis_LY:
        case Gamepad_Axis_RX:
        case Gamepad_Axis_RY:
            result = true;
            break;
    }

    return result;
}

static u32 gamepad_color(Gamepad_Input id){
    assert(id < Gamepad_Input_Max);

    u32 normal    = 0xffff0000;
    u32 highlight = 0xff0000ff;

    u32 result = normal;
    f32 value = g_gamepad_status[id];
    if(is_stick(id)){
        if(absf(value) > 3000) // TODO: We'll need to use a different deadzone value when we switch to using a -1 to 1 range for input values.
            result = highlight;
    }
    else{
        if(value != 0)
            result = highlight;
    }

    return result;
}

static Rect gamepad_bounds(Gamepad_Input id, Display_Info display){
    assert(id < Gamepad_Input_Max);

    Vec2 center = v2_muls((Vec2){display.window_width, display.window_height}, 0.5f);

    Vec2 extents = v2(20, 20);
    Vec2 spacing = v2_muls(extents, 2);

    Vec2 btn_center = v2_add(v2(250, 0), center);
    Vec2 l_stick_center = v2_add(v2(-250, 0), center);

    Rect result;
    switch(id){
        default: result = (Rect){v2(0, 0), v2(0, 0)};

        case Gamepad_Button_A: result = (Rect){v2_add(btn_center, v2(0, -spacing.y)), extents}; break;
        case Gamepad_Button_Y: result = (Rect){v2_add(btn_center, v2(0,  spacing.y)), extents}; break;
        case Gamepad_Button_X: result = (Rect){v2_add(btn_center, v2(-spacing.x, 0)), extents}; break;
        case Gamepad_Button_B: result = (Rect){v2_add(btn_center, v2( spacing.x, 0)), extents}; break;

        case Gamepad_Axis_LX: result = (Rect){l_stick_center, v2(extents.x*4.0f, extents.y)}; break;
        case Gamepad_Axis_LY: result = (Rect){l_stick_center, v2(extents.x, extents.y*4.0f)}; break;
    }

    return result;
}

int main(){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    u32 display_flags = Display_Flag_HW_Rendering;
    bool running = display_begin("Pad", 1024, 768, display_flags) && draw_begin(&memory);
    gamepad_begin(NULL, &memory);

    fmt_msg("{0}\n", fmt_i(0));

    memset(g_gamepad_status, 0, Array_Len(g_gamepad_status)*sizeof(f32));

    while(running){
        size_t memory_marker = buffer_frame_begin(&memory);

        Event event;
        while(display_next_event(&event)){
            switch(event.type){
                default: break;

                case Event_Type_Window_Close:{
                    running = false;
                } break;

                case Event_Type_Mouse_Motion:{
                    Event_Mouse_Motion *motion = &event.mouse_motion;

                } break;

                case Event_Type_Key:{
                    Event_Key *key = &event.key;
                    running = key->id != Key_ID_Escape;
                } break;
            }
        }

        gamepad_update(&memory);
        if(gamepad_get_count() > 0){
            Gamepad_Event pad_evt;
            while(gamepad_next_event(0, &pad_evt)){
                if(!is_stick(pad_evt.id) || absf(pad_evt.value) > 3000){
                    /*size_t marker = buffer_frame_begin(&memory);*/
                    String name = gamepad_get_event_string(pad_evt, &memory);
                    fmt_msg("{0}: {1}\n", fmt_cstr(name.text), fmt_f(pad_evt.value));
                    /*buffer_frame_end(&memory, marker);*/
                }

                if(pad_evt.type == Gamepad_Event_Input){
                    g_gamepad_status[pad_evt.id] = pad_evt.value;
                }
            }
        }

        Display_Info display = display_get_info();
        draw_frame_begin();
        draw_set_layer(Draw_Layer_World);

        draw_rect(gamepad_bounds(Gamepad_Button_A, display), gamepad_color(Gamepad_Button_A));
        draw_rect(gamepad_bounds(Gamepad_Button_Y, display), gamepad_color(Gamepad_Button_Y));
        draw_rect(gamepad_bounds(Gamepad_Button_B, display), gamepad_color(Gamepad_Button_B));
        draw_rect(gamepad_bounds(Gamepad_Button_X, display), gamepad_color(Gamepad_Button_X));
        draw_rect(gamepad_bounds(Gamepad_Axis_LX, display), gamepad_color(Gamepad_Axis_LX));
        draw_rect(gamepad_bounds(Gamepad_Axis_LY, display), gamepad_color(Gamepad_Axis_LY));

        draw_frame_end();

        display_flip_backbuffer();
        display_end_frame();

        buffer_frame_end(&memory, memory_marker);
    }

    gamepad_end();
    draw_end();
    display_end();
    return 0;
}
