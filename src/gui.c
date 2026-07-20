//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
TODO:
    - Panels need unique IDs
*/

#include "gui.h"
#include "draw.h"

#define Gui_Button_Padding      4
#define Gui_Window_Border_Size  4
#define Gui_Default_Margin      4
#define Gui_Window_Min_Width    400
#define Gui_Window_Min_Height   120
#define Gui_Window_Resize_Slack 4 // Additional space for grabbing window border for resize operation

#define Gui_Border_Color 0xffa86b33
#define Gui_Work_Color   0xfffae6d9

Cbed_API void gui_init(Gui *gui){
    gui->panels.next = &gui->panels;
    gui->panels.prev = &gui->panels;
}

static Gui_Panel *gui__get_hover_panel(Gui *gui){
    Gui_Panel *result = NULL;
    gui_iterate_panels(gui, panel){
        if(is_point_inside_rect(gui->cursor, panel->bounds)){
            result = panel;
        }
    }

    return result;
}

static Vec2 gui__get_real_size(Vec2 target_size, u32 canvas_w, u32 canvas_h){
    Vec2 result = target_size;

    if(target_size.x <= 1.0f){
        result.x = target_size.x*(f32)canvas_w;
    }

    if(target_size.y <= 1.0f){
        result.y = target_size.y*(f32)canvas_h;
    }

    return result;
}

static f32 gui__get_titlebar_height(Font* font){
    f32 result = font->metrics->height + Gui_Window_Border_Size*2;
    return result;
}

static Rect gui__get_titlebar_bounds(Gui_Panel* panel, Font* font){
    assert(panel->flags & Gui_Panel_Flag_Floating);
    auto title_bar_height = gui__get_titlebar_height(font);
    auto r      = panel->bounds;
    auto min_p  = v2(rect_left(r), rect_top(r) - title_bar_height);
    auto result = rect_from_min_wh(min_p, rect_width(r), title_bar_height);
    return result;
}

static Rect gui__get_work_area(Gui_Panel* panel, Font* font){
    if(panel->flags & Gui_Panel_Flag_Floating){
        Rect r = rect_cut_top(panel->bounds, gui__get_titlebar_height(font));
        r = rect_shrink(r, v2(Gui_Window_Border_Size, Gui_Window_Border_Size));

        /*
        if(should_scroll(window, font)){
            result = rect_cut_right(result, Scrollbar_Size);
        }*/

        return r;
    }
    else{
        Rect result = panel->bounds;
        return result;
    }
}

Cbed_API void gui_update(Gui *gui, u32 canvas_w, u32 canvas_h, f32 dt){
    // Auto-layout pass.
    f32 pen_y = canvas_h;
    gui_iterate_panels(gui, panel){
        Vec2 size = gui__get_real_size(panel->target_size, canvas_w, canvas_h);

        if(panel->flags & Gui_Panel_Flag_Floating){
            if(!(panel->flags & Gui_Panel_Flag_Initialized)){
                panel->bounds.center = v2(canvas_w*0.5f, canvas_h*0.5f);
            }

            if(size.x == 0){
                size.x = 360;
            }

            if(size.y == 0){
                size.y = 360;
            }
            panel->bounds.extents = v2_muls(size, 0.5f);
        }
        else{
            // TODO: Ordinarily the height would be determined by the elements inside the
            // panel + margins. For now, we just fake it.
            size.y = 32;

            Vec2 extents = v2_muls(size, 0.5f);
            pen_y -= extents.y;
            panel->bounds = (Rect){v2(extents.x, pen_y), extents};
            pen_y -= extents.y;
        }

        panel->flags |= Gui_Panel_Flag_Initialized;
    }
}

Cbed_API bool gui_handle_event(Gui *gui, Event *event){
    bool result = false;
    switch(event->type){
        default: break;

        case Event_Type_Mouse_Motion:{
            Event_Mouse_Motion *motion = &event->mouse_motion;
            gui->cursor = v2(motion->pixel_x, motion->pixel_y);

            switch(gui->status){
                default: break;

                case Gui_Status_Dragging:{
                    Gui_Panel *panel = gui->event_panel;
                    panel->bounds.center = v2_add(gui->click_offset, gui->cursor);
                } break;
            }
        } break;

        case Event_Type_Button:{
            Event_Button *btn = &event->button;
            if(btn->id == Button_ID_Mouse_Left){
                Gui_Panel *panel = gui__get_hover_panel(gui);
                if(panel && btn->pressed){
                    Rect bounds = gui__get_titlebar_bounds(panel, gui->font);
                    if(is_point_inside_rect(gui->cursor, bounds)){
                        gui->status = Gui_Status_Dragging;
                        gui->click_offset = v2_sub(panel->bounds.center, gui->cursor);
                        gui->event_panel = panel;
                    }
                }
                else{
                    gui->status = Gui_Status_Normal;
                }
            }
        } break;
    }

    return result;
}

Cbed_API void gui_draw(Gui *gui){
    Gui_Panel *sentinel = (Gui_Panel*)&gui->panels;
    gui_iterate_panels(gui, panel){
        draw_rect(panel->bounds, 0xff000000);
        draw_rect(rect_shrink(panel->bounds, v2(1, 1)), Gui_Border_Color);

        if(panel->flags & Gui_Panel_Flag_Floating){
            Font *font = gui->font;

            Rect work_area = gui__get_work_area(panel, font);
            draw_rect(rect_expand(work_area, v2(1, 1)), 0xff000000);
            draw_rect(work_area, Gui_Work_Color);

            String title = str_lit("Panel");
            if(panel->title.size){
                title = panel->title;
            }

            Rect title_bounds = gui__get_titlebar_bounds(panel, font);
            Vec2 baseline = font_align_text(font, Font_Align_Center, title.text, title.size, title_bounds);
            draw_text(baseline, 0xff000000, font, title.text, title.size);
        }
    }
}

Cbed_API Gui_Panel *gui_begin_panel(Gui *gui, void *data, size_t data_len, u32 flags){
    assert(data_len >= sizeof(Gui_Panel));

    Gui_Panel *panel_sentinel = (Gui_Panel*)&gui->panels;

    Gui_Panel *panel = (Gui_Panel *)data;
    panel->next = panel_sentinel->next;
    panel->prev = panel_sentinel;
    panel_sentinel->next->prev = panel;
    panel_sentinel->next = panel;

    panel->flags = flags;
    panel->buffer_length = data_len;
    panel->buffer_count = sizeof(Gui_Panel);
    return panel;
}

void gui_end_panel(Gui_Panel *panel){

}

Cbed_API void gui_button(Gui_Panel *panel, Gui_ID id, String text){

}
