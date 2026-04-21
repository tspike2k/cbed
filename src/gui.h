//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_GUI_H
#define CEABED_GUI_H

#include "common.h"
#include "math.h"
#include "display.h"

typedef Gui_ID u32;
#define gui_id(n) ((__LINE__ << 16) | n)

typedef struct{
    void *next;
    void *prev;
} Gui_Sentinel;

// TODO: Make this a Gui_Event instead?
typedef enum Gui_Status{
    Gui_Status_Normal,
    Gui_Status_Dragging,
    Gui_Status_Resizing,
} Gui_Status;

typedef struct Gui_Panel {
    struct Gui_Panel *next;
    struct Gui_Panel *prev;

    Vec2 target_size;
    Rect bounds;

    size_t buffer_length;
    size_t buffer_count;
    void  *buffer[0];
} Gui_Panel;

typedef struct{
    u32 user_id;
} Gui_Action;

typedef struct {
    u32          next_id;
    Gui_Sentinel panels;
    Vec2         cursor;

    Vec2 click_offset;
    Gui_Status status;
} Gui;

Ceabed_API bool gui_handle_event(Gui *gui, Event *event);
Ceabed_API void gui_update(Gui *gui, f32 dt);
Ceabed_API void gui_draw(Gui *gui);

Ceabed_API Gui_Panel *gui_begin_panel(Gui_Def *def, void *data, size_t data_len, u32 flags);
Ceabed_API void       gui_end_panel(Gui_Def *def, Gui_Panel *panel);
Ceabed_API void gui_button(Gui *gui, u32 user_id, String text);

#endif // CEABED_GUI_H
