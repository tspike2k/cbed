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

typedef u32 Gui_Def_Info;

enum Gui_Item_Type{
    Gui_Item_Type_None,
    Gui_Item_Type_Button,
};

typedef struct{
    u32  id;
    u32  user_id;
    Rect bounds;
} Gui_Item;

typedef struct{
    void *next;
    void *prev;
} Gui_Sentinel;

typedef enum Gui_Status{
    Gui_Status_Normal,
    Gui_Status_Dragging,
    Gui_Status_Resizing,
} Gui_Status;

typedef struct Gui_Panel {
    struct Gui_Panel *next;
    struct Gui_Panel *prev;

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

typedef struct{
    Gui *gui;
    Gui_Panel *panel;
} Gui_Def;

Ceabed_API void gui_update(Gui *gui, f32 dt);
Ceabed_API bool gui_handle_event(Gui *gui, Event *event);
Ceabed_API void gui_draw(Gui *gui);

Ceabed_API Gui_Def gui_begin_def(Gui *gui);
Ceabed_API void gui_end_def(Gui_Def *gui_def);
Ceabed_API void gui_begin_panel(Gui_Def *def, void *data, size_t data_len, Rect bounds, u32 flags);
Ceabed_API void gui_end_panel(Gui_Def *def);
Ceabed_API void gui_button(Gui *gui, u32 user_id, String text);

#endif // CEABED_GUI_H
