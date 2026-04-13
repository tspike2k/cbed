//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "gui.h"
#include "draw.h"

#define gui__iterate_panels(gui, panel) for(Gui_Panel *panel = (Gui_Panel *)gui->panels.next; \
    panel != (Gui_Panel *)&gui->panels; \
    panel = panel->next)

Ceabed_API void gui_init(Gui *gui){
    gui->panels.next = &gui->panels;
    gui->panels.prev = &gui->panels;
}

static Gui_Panel *gui__get_hover_panel(Gui *gui){
    return (Gui_Panel *)gui->panels.next;
}

Ceabed_API void gui_update(Gui *gui, f32 dt){

}

Ceabed_API bool gui_handle_event(Gui *gui, Event *event){
    bool result = false;
    switch(event->type){
        default: break;

        case Event_Type_Mouse_Motion:{
            Event_Mouse_Motion *motion = &event->mouse_motion;
            gui->cursor = v2(motion->pixel_x, motion->pixel_y);

            switch(gui->status){
                default: break;

                case Gui_Status_Dragging:{
                    Gui_Panel *panel = gui__get_hover_panel(gui);
                    panel->bounds.center = v2_add(gui->click_offset, gui->cursor);
                } break;
            }
        } break;

        case Event_Type_Button:{
            Event_Button *btn = &event->button;
            if(btn->id == Button_ID_Mouse_Left){
                Gui_Panel *panel = gui__get_hover_panel(gui);
                if(btn->pressed){
                    gui->status = Gui_Status_Dragging;
                    gui->click_offset = v2_sub(panel->bounds.center, gui->cursor);
                }
                else{
                    gui->status = Gui_Status_Normal;
                }
            }
        } break;
    }

    return result;
}

Ceabed_API void gui_draw(Gui *gui){
    Gui_Panel *sentinel = (Gui_Panel*)&gui->panels;
    gui__iterate_panels(gui, panel){
        draw_rect(panel->bounds, 0xffff0000);
    }
}

Ceabed_API Gui_Def gui_begin_def(Gui *gui){
    Gui_Def result = {gui};
    return result;
}

Ceabed_API void gui_end_def(Gui_Def *gui_def){

}

Ceabed_API void gui_begin_panel(Gui_Def *def, void *data, size_t data_len, Rect bounds, u32 flags){
    assert(data_len >= sizeof(Gui_Panel));

    Gui *gui = def->gui;
    Gui_Panel *panel_sentinel = (Gui_Panel*)&gui->panels;

    Gui_Panel *panel = (Gui_Panel *)data;
    panel->bounds = bounds;
    panel->next = panel_sentinel->next;
    panel->prev = panel_sentinel;
    panel_sentinel->next->prev = panel;
    panel_sentinel->next = panel;

    panel->buffer_length = data_len;
    panel->buffer_count = sizeof(Gui_Panel);
    def->panel = panel;
}

Ceabed_API void gui_end_panel(Gui_Def *def){
    def->panel = NULL;
}

Ceabed_API void gui_button(Gui *gui, u32 user_id, String text){

}
