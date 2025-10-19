//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "common.h"
#include "display.h"
#include <stdint.h>
#include <string.h>

/*
TODO:
    - Should we rename this to desktop.d? events.d? pal.d (Platform Abstraction Layer)?
    - Add function to copy to clipboard

*/

#ifdef __gnu_linux__
//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h> // free

typedef struct{
    Window handle;
    uint32_t width;
    uint32_t height;

    // For software rendering
    // TODO: The buffer and the width/height can be accessed directly from the XImage. Use those instead!
    uint32_t *backbuffer_pixels;
    XImage   *backbuffer;
} Xlib_Window;

typedef struct{
    Display *display;
    Atom     atom_WMState;
    Atom     atom_WMStateFullscreen;
    Atom     atom_WMDeleteWindow;
    Atom     atom_WMIcon;
    Atom     atom_clipboard;
    XIC      input_context;
    XIM      input_method;
    int      last_mouse_x;
    int      last_mouse_y;
    int      libXI_extension_opcode;
    int      libXI_master_pointer_device;
    bool     use_XI2_for_mouse;
    void*    event_data_to_free;
    char     text_buffer[32];
    Cursor   hidden_cursor;
    Pixmap   hidden_cursor_pixmap;
    Visual  *visual;
    int      color_depth;
    GC       graphics_context;

    Xlib_Window window;

#if 0
    // For hardware rendering
    glXCreateContextAttribsARBFunc glxCreateContextAttribsARB;
    glXSwapIntervalEXTFunc         glxSwapIntervalEXT;
    glXSwapIntervalMESAFunc        glxSwapIntervalMESA;
    glXSwapIntervalSGIFunc         glxSwapIntervalSGI;

    // TODO: These are shared between windows, right?
    GLXContext  g_glx_context;
    GLXFBConfig g_fb_config;
#endif
} Xlib;

static Xlib g__display;

static void display__create_backbuffer(Xlib_Window *window){
    if(window->backbuffer)
        XDestroyImage(window->backbuffer);

    window->backbuffer_pixels = (uint32_t*)calloc(1, window->width*window->height*sizeof(uint32_t));

    // TODO: Error handling
    window->backbuffer = XCreateImage(g__display.display, g__display.visual,
        g__display.color_depth, ZPixmap, 0, (char*)window->backbuffer_pixels,
        window->width, window->height, 32, 0);
    assert(window->backbuffer);
}

static Xlib_Window display__open_window(const char *window_title, uint32_t width, uint32_t height, uint32_t flags){
    Xlib *s = &g__display;

    // TODO: Find out once and for all what a "visual" and "gc" are and how they relate to software/hardware rendering.
    Xlib_Window result = {};

    // NOTE: Setting the window attribute "bit_gravity" to StaticGravity helps prevent flickering
    // when resizing the window. See here for more information:
    // https://handmade.network/forums/articles/t/2834-tutorial_a_tour_through_xlib_and_related_technologies
    uint32_t attributes_mask = CWEventMask|CWBackPixel|CWBitGravity|CWBackPixmap;
    XSetWindowAttributes attributes = {};
    attributes.event_mask = FocusChangeMask| ExposureMask | StructureNotifyMask
        | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
        | PointerMotionMask;
    attributes.background_pixmap = None; // TODO: Background pixmap? Does this mean we can make a framebuffer here?
    attributes.bit_gravity = StaticGravity;

    // The "screen" is a render target. It seems safe to use the default screen for the given display.
    int default_screen = DefaultScreen(s->display);

    Window xwindow = XCreateWindow(
        s->display, RootWindow(s->display, default_screen), 0, 0, width, height, 0,
        s->color_depth, InputOutput, s->visual, attributes_mask, &attributes
    );
    if(xwindow){
        result.handle  = xwindow;
        result.width   = width;
        result.height  = height;

        if(!(flags & Display_Flag_HW_Rendering)){
            display__create_backbuffer(&result);
        }

        XSetWMProtocols(s->display, xwindow, &s->atom_WMDeleteWindow, 1);
        XStoreName(s->display, xwindow, window_title);
        XMapWindow(s->display, xwindow);
        XSync(s->display, False);
    }
    else{
        fmt_msg_puts("Unable to create Xlib window.\n");
    }

    return result;
}

static bool display__process_event(XEvent *xevt, Event *evt){
    Xlib *s = &g__display;

    bool event_translated = false;

    switch(xevt->type){
        default: break;

#if 0
        case Expose:{
            event_translated = true;
            evt->type = Event_Type_Repaint;
        } break;
#endif

        case ButtonPress:
        case ButtonRelease:{
            auto button = &evt->button;

            button->type = Event_Type_Button;
            button->pressed = xevt->type == ButtonPress;
            event_translated = true;
            switch(xevt->xbutton.button){
                default:
                    event_translated = false;
                    break;

                case Button1:
                    button->id = Button_ID_Mouse_Left;
                    break;

                case Button2:
                    button->id = Button_ID_Mouse_Middle;
                    break;

                case Button3:
                    button->id = Button_ID_Mouse_Right;
                    break;
            }
        } break;

        case MotionNotify:{
            auto motion = &evt->mouse_motion;
            motion->type = Event_Type_Mouse_Motion;
            // TODO: Find out what motion hints are and if we need them.
            motion->pixel_x = xevt->xmotion.x;
            motion->pixel_y = xevt->xmotion.y;
            event_translated = true;
        } break;

        case ClientMessage:{
            if(s->atom_WMDeleteWindow == (Atom)xevt->xclient.data.l[0]){
                event_translated = true;
                evt->type = Event_Type_Window_Close;
            }
        } break;

        case ConfigureNotify:{
            s->window.width  = xevt->xconfigure.width;
            s->window.height = xevt->xconfigure.height;
            display__create_backbuffer(&s->window);
        } break;

        case KeyPress:
        case KeyRelease:{
            KeySym keycode = XLookupKeysym(&xevt->xkey, 0);
            bool is_repeat = false;
            bool just_pressed = xevt->type == KeyPress;

            // When a keyboard key is first pressed, a Key Press event is generated. If the key is held
            // down, Operating Systems will typically generate additional Key Press events at regular
            // intervals. Xlib does this, as most users would expect. Before each repeat Key Press
            // event, however, a Key Release event is misleadingly generated.
            //
            // Calling this function will check for these events, returning true if one was found. It
            // will then consume the Key Release event, replacing the "evt" parameter with the repeat
            // Key Press event.
            //
            // See here for more info:
            // https://groups.google.com/forum/#!topic/pyglet-users/KaF8RQb-ifc
            // https://stackoverflow.com/questions/2100654/ignore-auto-repeat-in-x11-applications
            // https://github.com/glfw/glfw/blob/master/src/x11_window.c
            if(xevt->type == KeyRelease && XEventsQueued(s->display, QueuedAlready)){
                XEvent peek_evt;
                XPeekEvent(s->display, &peek_evt);
                if(peek_evt.type == KeyPress && XLookupKeysym(&peek_evt.xkey, 0) == keycode && (peek_evt.xkey.time - xevt->xkey.time) < 20){
                    XNextEvent(s->display, xevt);
                    is_repeat = true;
                }
            }

            if(xevt->xkey.state & ControlMask){
                evt->key.modifier |= Key_Modifier_Ctrl;
            }

            evt->type          = Event_Type_Key;
            evt->key.pressed   = just_pressed;
            evt->key.is_repeat = is_repeat;

            event_translated = true;
            switch(keycode){
                default:
                    event_translated = false;
                    break;

                case XK_A:
                case XK_a:
                    evt->key.id = Key_ID_A; break;

                case XK_B:
                case XK_b:
                    evt->key.id = Key_ID_B; break;

                case XK_C:
                case XK_c:
                    evt->key.id = Key_ID_C; break;

                case XK_D:
                case XK_d:
                    evt->key.id = Key_ID_D; break;

                case XK_E:
                case XK_e:
                    evt->key.id = Key_ID_E; break;

                case XK_F:
                case XK_f:
                    evt->key.id = Key_ID_F; break;

                case XK_G:
                case XK_g:
                    evt->key.id = Key_ID_G; break;

                case XK_H:
                case XK_h:
                    evt->key.id = Key_ID_H; break;

                case XK_I:
                case XK_i:
                    evt->key.id = Key_ID_I; break;

                case XK_J:
                case XK_j:
                    evt->key.id = Key_ID_J; break;

                case XK_K:
                case XK_k:
                    evt->key.id = Key_ID_K; break;

                case XK_L:
                case XK_l:
                    evt->key.id = Key_ID_L; break;

                case XK_M:
                case XK_m:
                    evt->key.id = Key_ID_M; break;

                case XK_N:
                case XK_n:
                    evt->key.id = Key_ID_N; break;

                case XK_O:
                case XK_o:
                    evt->key.id = Key_ID_O; break;

                case XK_P:
                case XK_p:
                    evt->key.id = Key_ID_P; break;

                case XK_Q:
                case XK_q:
                    evt->key.id = Key_ID_Q; break;

                case XK_R:
                case XK_r:
                    evt->key.id = Key_ID_R; break;

                case XK_S:
                case XK_s:
                    evt->key.id = Key_ID_S; break;

                case XK_T:
                case XK_t:
                    evt->key.id = Key_ID_T; break;

                case XK_U:
                case XK_u:
                    evt->key.id = Key_ID_U; break;

                case XK_V:
                case XK_v:
                    evt->key.id = Key_ID_V; break;

                case XK_W:
                case XK_w:
                    evt->key.id = Key_ID_W; break;

                case XK_X:
                case XK_x:
                    evt->key.id = Key_ID_X; break;

                case XK_Y:
                case XK_y:
                    evt->key.id = Key_ID_Y; break;

                case XK_Z:
                case XK_z:
                    evt->key.id = Key_ID_Z; break;

                case XK_0:
                    evt->key.id = Key_ID_0; break;

                case XK_1:
                    evt->key.id = Key_ID_1; break;

                case XK_2:
                    evt->key.id = Key_ID_2; break;

                case XK_3:
                    evt->key.id = Key_ID_3; break;

                case XK_4:
                    evt->key.id = Key_ID_4; break;

                case XK_5:
                    evt->key.id = Key_ID_5; break;

                case XK_6:
                    evt->key.id = Key_ID_6; break;

                case XK_7:
                    evt->key.id = Key_ID_7; break;

                case XK_8:
                    evt->key.id = Key_ID_8; break;

                case XK_9:
                    evt->key.id = Key_ID_9; break;

                case XK_F1:
                    evt->key.id = Key_ID_F1; break;

                case XK_F2:
                    evt->key.id = Key_ID_F2; break;

                case XK_F3:
                    evt->key.id = Key_ID_F3; break;

                case XK_F4:
                    evt->key.id = Key_ID_F4; break;

                case XK_F5:
                    evt->key.id = Key_ID_F5; break;

                case XK_F6:
                    evt->key.id = Key_ID_F6; break;

                case XK_F7:
                    evt->key.id = Key_ID_F7; break;

                case XK_F8:
                    evt->key.id = Key_ID_F8; break;

                case XK_F9:
                    evt->key.id = Key_ID_F9; break;

                case XK_F10:
                    evt->key.id = Key_ID_F10; break;

                case XK_F11:
                    evt->key.id = Key_ID_F11; break;

                case XK_F12:
                    evt->key.id = Key_ID_F12; break;

                case XK_Up:
                    evt->key.id = Key_ID_Arrow_Up; break;

                case XK_Down:
                    evt->key.id = Key_ID_Arrow_Down; break;

                case XK_Left:
                    evt->key.id = Key_ID_Arrow_Left; break;

                case XK_Right:
                    evt->key.id = Key_ID_Arrow_Right; break;

                case XK_Return:
                    evt->key.id = Key_ID_Enter; break;

                case XK_Delete:
                    evt->key.id = Key_ID_Delete; break;

                case XK_Escape:
                    evt->key.id = Key_ID_Escape; break;

                case XK_BackSpace:
                    evt->key.id = Key_ID_Backspace; break;
            }
        } break;

    }
    return event_translated;
}

Display_Backbuffer display_get_sw_backbuffer(){
    Display_Backbuffer result = {};
    result.width  = g__display.window.width;
    result.height = g__display.window.height;
    result.pixels = g__display.window.backbuffer_pixels;
    return result;
}

void display_flip_backbuffer(){
    Xlib *s = &g__display;
    XPutImage(
        s->display, s->window.handle, s->graphics_context,
        s->window.backbuffer,
        0, 0, 0, 0, s->window.width, s->window.height
    );
}

bool display_next_event(Event *event){
    // Some translated events need to pass data allocated by Xlib to the caller. This data
    // needs to be freed, but the caller shouldn't have to deal with that (especially since
    // it could be different on other platforms). This is the simplest way to support that.
    //
    // IMPORTANT: g_event_data_to_free should ONLY be set when an event has been succesfully
    // translated. If the translation failed, the data should be freed on failure, not here.
    Xlib *s = &g__display;
    if(s->event_data_to_free){
        free(s->event_data_to_free);
        s->event_data_to_free = NULL;
    }

    bool event_translated = false;
    auto display = s->display;
    while(!event_translated && XEventsQueued(display, QueuedAlready)){
        XEvent xevt;
        XNextEvent(display, &xevt);
        event_translated = display__process_event(&xevt, event);
    }

    event->consumed = false;
    return event_translated;
}

static void display__close_window(Xlib_Window* w){
    if(w->handle){
        XDestroyWindow(g__display.display, w->handle);
        w->handle = 0;
    }
}

bool display_begin(const char *window_title, uint32_t width, uint32_t height, uint32_t window_flags){
    Xlib *s = &g__display;

    ceabed_begin();

    s->display = XOpenDisplay(NULL);
    if(!s->display){
        fmt_msg_puts("Unable to open X11 display. Aborting.\n");
        return false;
    }

    s->atom_WMState           = XInternAtom(s->display, "_NET_WM_STATE", False);
    s->atom_WMStateFullscreen = XInternAtom(s->display, "_NET_WM_STATE_FULLSCREEN", False);
    s->atom_WMDeleteWindow    = XInternAtom(s->display, "WM_DELETE_WINDOW", False);
    s->atom_WMIcon            = XInternAtom(s->display, "_NET_WM_ICON", False);
    s->atom_clipboard         = XInternAtom(s->display, "CLIPBOARD", False);

    int x11_screen = XDefaultScreen(s->display);
    if(window_flags & Display_Flag_HW_Rendering){
        assert(0);
    }
    else{
        XVisualInfo visual_info = {};
        if(!XMatchVisualInfo(s->display, x11_screen, 24, TrueColor, &visual_info)){
            fmt_msg_puts("Unable to get VisualInfo from Xlib. Aborting.\n");
            return false;
        }

        if(visual_info.class != TrueColor){
            fmt_msg_puts("Unable to get TrueColor Visual from Xlib. Aborting.\n");
            return false;
        }

        if(visual_info.red_mask != 0xFF0000 || visual_info.green_mask != 0x00FF00 || visual_info.blue_mask != 0x0000FF){
            fmt_msg_puts("Unable to get color channels with the required mask. Aborting.\n");
            return false;
        }

        s->graphics_context = XDefaultGC(s->display, x11_screen);
        s->visual           = visual_info.visual;
        s->color_depth      = visual_info.depth;
    }

    s->window = display__open_window(window_title, width, height, window_flags);

#if 0
    int screen = g_xlib_window.screen;
    const char* glx_extension_string = glXQueryExtensionsString(g_x11_display, screen);
    load_glx_extensions(glx_extension_string[0 .. strlen(glx_extension_string)]);

    XextErrorHandler default_xext_error_handler =
        XSetExtensionErrorHandler(&xext_error_handler);

    g_use_XI2_for_mouse = request_XInput2_mouse_events();

    XSync(g_x11_display, False);
    XSetExtensionErrorHandler(default_xext_error_handler);

    if(glxCreateContextAttribsARB){
        // NOTE(tspike): Temporarily stub out the X11 error handler with our own in case context creation fails.
        XErrorHandler defaultX11ErrorHandler = XSetErrorHandler(&stub_x11_error_handler);

        // NOTE(tspike): Setting GLX_CONTEXT_FLAGS_ARB to GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
        // appears to be bad practice. The official OpenGL wiki states you should NEVER do it:
        // https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
        debug{
            int[9] glxContextAttribs = [
                GLX_CONTEXT_MAJOR_VERSION_ARB, Target_OpenGL_Version_Major,
                GLX_CONTEXT_MINOR_VERSION_ARB, Target_OpenGL_Version_Minor,
                GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
                None
            ];
        }
        else{
            int[7] glxContextAttribs = [
                GLX_CONTEXT_MAJOR_VERSION_ARB, Target_OpenGL_Version_Major,
                GLX_CONTEXT_MINOR_VERSION_ARB, Target_OpenGL_Version_Minor,
                GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                None
            ];
        }

        // TODO: Error checking
        g_glx_context = glxCreateContextAttribsARB(g_x11_display, g_fb_config, null, True, &glxContextAttribs[0]);

        // NOTE(tspike): Call XSync to force X11 to process errors and send them to our error handler
        XSync(g_x11_display, False);
        XSetErrorHandler(defaultX11ErrorHandler);
    }

    // TODO: Fallback to software rendering if we can't get an OpenGL context
    if (!g_glx_context){
        log("Unable to create OpenGL context. Exiting.\n"); // TODO: Better logging
        //log("Unable to create OpenGL {0}.{1} context. Exiting.\n", fmt_u(TARGET_GL_VERSION_MAJOR), fmt_u(TARGET_GL_VERSION_MINOR));
        return false;
    }
    glXMakeCurrent(g_x11_display, g_xlib_window.handle, g_glx_context);

    load_opengl_functions(cast(OpenGL_Load_Sym_Func)&glXGetProcAddressARB);

    XColor hidden_cursor_color = void;
    clear_to_zero(hidden_cursor_color);
    char[1] hidden_cursor_data = void;
    hidden_cursor_data[0] = 0;
    g_x11_hidden_cursor_pixmap = XCreateBitmapFromData(g_x11_display, g_xlib_window.handle, hidden_cursor_data.ptr, 1, 1);
    g_x11_hidden_cursor = XCreatePixmapCursor(
        g_x11_display, g_x11_hidden_cursor_pixmap, g_x11_hidden_cursor_pixmap, &hidden_cursor_color, &hidden_cursor_color, 0, 0
    );

    XWindow focus_window;
    int focus_window_revert;
    XGetInputFocus(g_x11_display, &focus_window, &focus_window_revert);
    g_has_focus = g_xlib_window.handle == focus_window;

    // We need to make sure that at least one FocusIn event is generated in order to ensure
    // g_has_focus is accurate.
    // TODO: Is there a better way to do this? Can we assume the window will have input focus?
    // Can we queury for which window has focus?
    XSetInputFocus(g_x11_display, DefaultRootWindow(g_x11_display), RevertToPointerRoot, CurrentTime);

    //log("OpenGL context: {0}\n", fmt_cstr((const char*)glGetString(GL_VERSION)));
    //log("OpenGL shader version: {0}\n", fmt_cstr((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION)));

    // TODO: Only make this fullscreen if the window_flags requests it.
    // TODO: Make this send_fullscreen_request and take the _NET_WM_STATE as a parameter.
    send_fullscreen_toggle_request();
    XSync(g_x11_display, True);
#endif
    return true;
}

void display_end(){
    Xlib * s = &g__display;

    if(s->hidden_cursor_pixmap){
        XFreePixmap(s->display, s->hidden_cursor_pixmap);
    }
    display__close_window(&s->window);
    //if(g_glx_context) glXDestroyContext(g_x11_display, g_glx_context);
    if(s->display){
        XCloseDisplay(s->display);
        s->display = NULL;
    }
    ceabed_end();
}

void display_end_frame(){
    XFlush(g__display.display);
}

//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------
#endif
