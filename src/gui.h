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

typedef u32 Gui_ID;
#define gui_id(n) ((__LINE__ << 16) | n)

enum{
    Gui_Panel_Flag_Initialized = (1 << 0),
    Gui_Panel_Flag_Floating    = (1 << 1),
};

#define gui_iterate_panels(gui, panel) for(Gui_Panel *panel = (Gui_Panel *)gui->panels.next; \
    panel != (Gui_Panel *)&gui->panels; \
    panel = panel->next)

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

typedef struct Gui_Panel Gui_Panel;

typedef struct {
    Gui_Sentinel panels;
    Vec2         cursor;

    Font *font;
    Gui_Panel *event_panel;
    Vec2 click_offset;
    Gui_Status status;
} Gui;

struct Gui_Panel{
    Gui_Panel *next;
    Gui_Panel *prev;

    Gui *gui;
    Rect bounds;
    Vec2 target_size;
    String title;
    u32  flags;

    size_t buffer_length;
    size_t buffer_count;
    void  *buffer[0];
};

Ceabed_API bool gui_handle_event(Gui *gui, Event *event);
Ceabed_API void gui_update(Gui *gui, u32 canvas_w, u32 canvas_h, f32 dt);
Ceabed_API void gui_draw(Gui *gui);

Ceabed_API Gui_Panel *gui_begin_panel(Gui *gui, void *data, size_t data_len, u32 flags);
Ceabed_API void       gui_end_panel(Gui_Panel *panel);
Ceabed_API void gui_button(Gui_Panel *panel, Gui_ID id, String text);

#endif // CEABED_GUI_H
