//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "display.h"
#include <string.h>
#include <assert.h>

/*
TODO:
    - Should we rename this to desktop.d? events.d? pal.d (Platform Abstraction Layer)?
    - Add function to copy to clipboard

*/

#ifdef OS_Linux
//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------

#include "opengl.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <stdlib.h> // free

enum{
    GLX_USE_GL           = 1,
    GLX_BUFFER_SIZE      = 2,
    GLX_LEVEL            = 3,
    GLX_RGBA             = 4,
    GLX_DOUBLEBUFFER     = 5,
    GLX_STEREO           = 6,
    GLX_AUX_BUFFERS      = 7,
    GLX_RED_SIZE         = 8,
    GLX_GREEN_SIZE       = 9,
    GLX_BLUE_SIZE        = 10,
    GLX_ALPHA_SIZE       = 11,
    GLX_DEPTH_SIZE       = 12,
    GLX_STENCIL_SIZE     = 13,
    GLX_ACCUM_RED_SIZE   = 14,
    GLX_ACCUM_GREEN_SIZE = 15,
    GLX_ACCUM_BLUE_SIZE	 = 16,
    GLX_ACCUM_ALPHA_SIZE = 17,
};

enum{
    GLX_CONFIG_CAVEAT           = 0x20,
    GLX_DONT_CARE               = 0xFFFFFFFF,
    GLX_X_VISUAL_TYPE           = 0x22,
    GLX_TRANSPARENT_TYPE        = 0x23,
    GLX_TRANSPARENT_INDEX_VALUE = 0x24,
    GLX_TRANSPARENT_RED_VALUE   = 0x25,
    GLX_TRANSPARENT_GREEN_VALUE = 0x26,
    GLX_TRANSPARENT_BLUE_VALUE  = 0x27,
    GLX_TRANSPARENT_ALPHA_VALUE = 0x28,
    GLX_WINDOW_BIT              = 0x00000001,
    GLX_PIXMAP_BIT              = 0x00000002,
    GLX_PBUFFER_BIT             = 0x00000004,
    GLX_AUX_BUFFERS_BIT         = 0x00000010,
    GLX_FRONT_LEFT_BUFFER_BIT   = 0x00000001,
    GLX_FRONT_RIGHT_BUFFER_BIT  = 0x00000002,
    GLX_BACK_LEFT_BUFFER_BIT    = 0x00000004,
    GLX_BACK_RIGHT_BUFFER_BIT   = 0x00000008,
    GLX_DEPTH_BUFFER_BIT        = 0x00000020,
    GLX_STENCIL_BUFFER_BIT      = 0x00000040,
    GLX_ACCUM_BUFFER_BIT        = 0x00000080,
    GLX_NONE                    = 0x8000,
    GLX_SLOW_CONFIG             = 0x8001,
    GLX_TRUE_COLOR              = 0x8002,
    GLX_DIRECT_COLOR            = 0x8003,
    GLX_PSEUDO_COLOR            = 0x8004,
    GLX_STATIC_COLOR            = 0x8005,
    GLX_GRAY_SCALE              = 0x8006,
    GLX_STATIC_GRAY             = 0x8007,
    GLX_TRANSPARENT_RGB         = 0x8008,
    GLX_TRANSPARENT_INDEX       = 0x8009,
    GLX_VISUAL_ID               = 0x800B,
    GLX_SCREEN                  = 0x800C,
    GLX_NON_CONFORMANT_CONFIG   = 0x800D,
    GLX_DRAWABLE_TYPE           = 0x8010,
    GLX_RENDER_TYPE             = 0x8011,
    GLX_X_RENDERABLE            = 0x8012,
    GLX_FBCONFIG_ID	            = 0x8013,
    GLX_RGBA_TYPE               = 0x8014,
    GLX_COLOR_INDEX_TYPE        = 0x8015,
    GLX_MAX_PBUFFER_WIDTH       = 0x8016,
    GLX_MAX_PBUFFER_HEIGHT      = 0x8017,
    GLX_MAX_PBUFFER_PIXELS      = 0x8018,
    GLX_PRESERVED_CONTENTS      = 0x801B,
    GLX_LARGEST_PBUFFER         = 0x801C,
    GLX_WIDTH                   = 0x801D,
    GLX_HEIGHT                  = 0x801E,
    GLX_EVENT_MASK              = 0x801F,
    GLX_DAMAGED                 = 0x8020,
    GLX_SAVED                   = 0x8021,
    GLX_WINDOW                  = 0x8022,
    GLX_PBUFFER                 = 0x8023,
    GLX_PBUFFER_HEIGHT          = 0x8040,
    GLX_PBUFFER_WIDTH           = 0x8041,
    GLX_RGBA_BIT                = 0x00000001,
    GLX_COLOR_INDEX_BIT         = 0x00000002,
    GLX_PBUFFER_CLOBBER_MASK    = 0x08000000,
};

enum{
    GLX_CONTEXT_DEBUG_BIT_ARB              = 0x00000001,
    GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB = 0x00000002,
    GLX_CONTEXT_MAJOR_VERSION_ARB          = 0x2091,
    GLX_CONTEXT_MINOR_VERSION_ARB          = 0x2092,
    GLX_CONTEXT_FLAGS_ARB                  = 0x2094,
    GLX_CONTEXT_CORE_PROFILE_BIT_ARB       = 0x00000001,
    GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB = 0x00000002,
    GLX_CONTEXT_PROFILE_MASK_ARB           = 0x9126,
};

typedef struct __GLXcontextRec __GLXcontextRec;
typedef __GLXcontextRec* GLXContext;
typedef XID GLXPixmap;
typedef XID GLXDrawable;
typedef struct __GLXFBConfigRec __GLXFBConfigRec;
typedef __GLXFBConfigRec* GLXFBConfig;
typedef XID GLXFBConfigID;
typedef XID GLXContextID;
typedef XID GLXWindow;
typedef XID GLXPbuffer;
typedef void (*GLXextFuncPtr)(void);

GLXFBConfig *glXChooseFBConfig(Display* dpy, int screen, const int *attribList, int *nitems);
XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config);
const char *glXQueryExtensionsString(Display *dpy, int screen);
Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx);
void glXDestroyContext(Display *dpy, GLXContext ctx);
void glXSwapBuffers(Display *dpy, GLXDrawable drawable);
GLXextFuncPtr glXGetProcAddressARB(const uint8_t *);

typedef GLXContext (*glXCreateContextAttribsARBFunc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef void (*glXSwapIntervalEXTFunc)(Display* display, GLXDrawable drawable, int interval);
typedef int (*glXSwapIntervalMESAFunc)(uint32_t interval) ;
typedef int (*glXSwapIntervalSGIFunc)(int interval);

static glXCreateContextAttribsARBFunc glxCreateContextAttribsARB;
static glXSwapIntervalEXTFunc         glxSwapIntervalEXT;
static glXSwapIntervalMESAFunc        glxSwapIntervalMESA;
static glXSwapIntervalSGIFunc         glxSwapIntervalSGI;

typedef struct{
    Window handle;
    uint32_t flags;
    uint32_t width;
    uint32_t height;

    // For software rendering
    // TODO: The buffer and the width/height can be accessed directly from the XImage. Use those instead!
    uint32_t *backbuffer_pixels;
    XImage   *backbuffer;
    Visual   *visual;
    int       screen;
    int       bit_depth;
    GC        graphics_context;
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

    Xlib_Window window;

#if 0
    // For hardware rendering
    glXCreateContextAttribsARBFunc glxCreateContextAttribsARB;
    glXSwapIntervalEXTFunc         glxSwapIntervalEXT;
    glXSwapIntervalMESAFunc        glxSwapIntervalMESA;
    glXSwapIntervalSGIFunc         glxSwapIntervalSGI;

#endif
    // TODO: These are shared between windows, right?
    GLXContext  glx_context;
    GLXFBConfig glx_fb_config;
    Colormap    colormap;
} Xlib;

static Xlib g__display;

static void display__create_backbuffer(Xlib_Window *window){
    if(window->backbuffer)
        XDestroyImage(window->backbuffer);

    window->backbuffer_pixels = (uint32_t*)calloc(1, window->width*window->height*sizeof(uint32_t));

    // TODO: Error handling
    window->backbuffer = XCreateImage(g__display.display, window->visual,
        window->bit_depth, ZPixmap, 0, (char*)window->backbuffer_pixels,
        window->width, window->height, 32, 0);
    assert(window->backbuffer);
}

static bool display__get_sw_visual_info(XVisualInfo *visual_info, int screen){
    bool result = false;
    // TODO: Look for a better match on the visual. We want to ensure that we get an RGBA
    // Visual.
    if(XMatchVisualInfo(g__display.display, screen, 24, TrueColor, visual_info)){
        result = true;
    }
    else{
        fmt_msg_puts("Unable to match visual for window.\n");
    }
    return result;
}

static bool display__str_vs_cstr(const char *s0, size_t s0_len, const char *s1){
    bool result = true;
    size_t i;
    for(i = 0; i < s0_len; i++){
        if(s0[i] != s1[i]){
            result = false;
            break;
        }
    }

    result = result && s1[i] == '\0';
    return result;
}

static int display__stub_xlib_error_handler(Display* display, XErrorEvent* ev){
    char buffer[256];
    XGetErrorText(display, ev->error_code, &buffer[0], 256);
    fmt_msg_puts(&buffer[0]);
    fmt_msg_puts("\n");
    return 0;
}

static bool display__init_glx(XVisualInfo *visual_info, int screen){
    Xlib *s = &g__display;

    const char *extension_string = glXQueryExtensionsString(s->display, screen);
    if(!extension_string) return false;

    //
    // Load GLX Extensions
    //
    const char *reader = extension_string;
    while(*reader != '\0'){
        const char *start = reader;
        size_t len = 0;
        while(*reader != '\0'){
            if(reader[0] == ' '){
                len = reader - start;
                reader++;
                break;
            }
            reader++;
        }

        if(display__str_vs_cstr(start, len, "GLX_EXT_swap_control")){
            glxSwapIntervalEXT = (glXSwapIntervalEXTFunc)glXGetProcAddressARB((uint8_t*)"glXSwapIntervalEXT");
        }
        else if(display__str_vs_cstr(start, len, "GLX_MESA_swap_control")){
            glxSwapIntervalMESA = (glXSwapIntervalMESAFunc)glXGetProcAddressARB((uint8_t*)"glXSwapIntervalMESA");
        }
        else if(display__str_vs_cstr(start, len, "GLX_SGI_swap_control")){
            glxSwapIntervalSGI = (glXSwapIntervalSGIFunc)glXGetProcAddressARB((uint8_t*)"glXSwapIntervalSGI");
        }
        else if(display__str_vs_cstr(start, len, "GLX_ARB_create_context")
        || display__str_vs_cstr(start, len, "GLX_ARB_create_context_profile")){
            if(!glxCreateContextAttribsARB){
                glxCreateContextAttribsARB = (glXCreateContextAttribsARBFunc)glXGetProcAddressARB((uint8_t*)"glXCreateContextAttribsARB");
            }
        }
    }

    //
    // Find OpenGL compatible framebuffer.
    //
    // NOTE: This is based on the code found at the openGL tutorial found here:
    // https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)
    int target_framebuffer_attribs[] =
    {
        GLX_X_RENDERABLE    , True,
        GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
        GLX_RED_SIZE        , 8,
        GLX_GREEN_SIZE      , 8,
        GLX_BLUE_SIZE       , 8,
        GLX_ALPHA_SIZE      , 8,
        GLX_DEPTH_SIZE      , 24,
        GLX_STENCIL_SIZE    , 8,
        GLX_DOUBLEBUFFER    , True,
        //GLX_SAMPLE_BUFFERS  , 1,
        //GLX_SAMPLES         , 4,
        None
    };

    if(glxCreateContextAttribsARB){
        int fb_count;
        GLXFBConfig *fb_list = glXChooseFBConfig(s->display, screen, &target_framebuffer_attribs[0], &fb_count);
        if(fb_count > 0){
            // TODO: Choose and store best visual/fbConfig
            s->glx_fb_config = fb_list[0];
            XVisualInfo *target_visual_info = glXGetVisualFromFBConfig(s->display, s->glx_fb_config); // TODO: Can this fail?

            // It seems we *must* create a colormap when using OpenGL. If we don't, we get a BadMatch error.
            s->colormap = XCreateColormap(s->display, RootWindow(s->display, target_visual_info->screen), target_visual_info->visual, AllocNone);

            *visual_info = *target_visual_info;
            XFree(target_visual_info);
            XFree(fb_list);

            fmt_msg_puts("Got framebuffer config for HW rendering.\n");

            // NOTE: Temporarily stub out the X11 error handler with our own in case context creation fails.
            XErrorHandler old_error_handler = XSetErrorHandler(display__stub_xlib_error_handler);

            // NOTE: Setting GLX_CONTEXT_FLAGS_ARB to GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
            // appears to be bad practice. The official OpenGL wiki states you should NEVER do it:
            // https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
            int glxContextAttribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, OpenGL_Version_Major,
                GLX_CONTEXT_MINOR_VERSION_ARB, OpenGL_Version_Minor,
                GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
#if !NDEBUG
                GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
                None
            };

            // TODO: Error checking
            s->glx_context = glxCreateContextAttribsARB(s->display, s->glx_fb_config, NULL, True, &glxContextAttribs[0]);
            if(!s->glx_context){
                fmt_msg_puts("Unable to create OpenGL context.\n");
            }

            // NOTE(tspike): Call XSync to force X11 to process errors and send them to our error handler
            XSync(s->display, False);
            XSetErrorHandler(old_error_handler);
        }
        else{
            fmt_msg_puts("Unable to get framebuffer list. Falling back to software rendering.\n");
        }
    }

    bool result = s->glx_context;
    return result;
}

static Xlib_Window display__open_window(const char *window_title, XVisualInfo * visual_info, uint32_t width, uint32_t height, uint32_t flags){
    Xlib *s = &g__display;

    // TODO: Find out once and for all what a "visual" and "gc" are and how they relate to software/hardware rendering.
    Xlib_Window result = {0};

    // NOTE: Setting the window attribute "bit_gravity" to StaticGravity helps prevent flickering
    // when resizing the window. See here for more information:
    // https://handmade.network/forums/articles/t/2834-tutorial_a_tour_through_xlib_and_related_technologies
    uint32_t attributes_mask = CWEventMask|CWBitGravity|CWBackPixmap;
    XSetWindowAttributes attributes = {0};
    attributes.event_mask = FocusChangeMask| ExposureMask | StructureNotifyMask
        | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
        | PointerMotionMask;
    attributes.background_pixmap = None; // TODO: Background pixmap? Does this mean we can make a framebuffer here?
    attributes.bit_gravity = StaticGravity;

    bool use_hw_rendering = (flags & Display_Flag_HW_Rendering);
    if(use_hw_rendering){
        assert(s->colormap);
        attributes_mask |= CWColormap;
        attributes.colormap = s->colormap;
    }
    else{
        attributes_mask |= CWBackPixel;
        attributes.background_pixel = BlackPixel(s->display, visual_info->screen);
    }

    Window window = XCreateWindow(
        s->display, RootWindow(s->display, visual_info->screen), 0, 0, width, height, 0,
        visual_info->depth, InputOutput, visual_info->visual, attributes_mask, &attributes
    );
    if(window){
        result.handle           = window;
        result.flags            = flags;
        result.screen           = visual_info->screen;
        result.bit_depth        = visual_info->depth;
        result.visual           = visual_info->visual;
        result.graphics_context = DefaultGC(s->display, visual_info->screen);
        result.width            = width;
        result.height           = height;

        if(!use_hw_rendering){
            display__create_backbuffer(&result);
        }

        XSetWMProtocols(s->display, window, &s->atom_WMDeleteWindow, 1);
        XStoreName(s->display, window, window_title);
        XMapWindow(s->display, window);
        XSync(s->display, False);
    }
    else{
        fmt_msg_puts("Unable to create Xlib window.\n");
    }

    return result;
}

static bool display__request_XInput2_mouse_events(){
    Xlib *s = &g__display;

    // NOTE: Much of this function is based around the ManyMouse library written by Ryan C. Gordon, released under the ZLIB license.
    bool succeeded = false;

    int first_event, first_error;
    if(XQueryExtension(s->display, "XInputExtension", &s->libXI_extension_opcode, &first_event, &first_error)){
        int version_major = 2;
        int version_minor = 0;
        if (XIQueryVersion(s->display, &version_major, &version_minor) == Success){
            XIEventMask evmask;
            u8 mask[3] = {0, 0, 0};

            XISetMask(&mask[0], XI_RawMotion);
            //XISetMask(mask.ptr, XI_RawButtonPress);
            //XISetMask(mask.ptr, XI_RawButtonRelease);

            // NOTE: We should only need to subscribe to master device events because the master device is supposed to forward
            // events generated by every associated slave device. However, when we do we never receive any XI_RawButtonRelease events.
            // So instead we'll subscribe to all devices and simply ignore events sent by the master pointer device.
            evmask.deviceid = XIAllDevices;
            evmask.mask_len = Array_Len(mask);
            evmask.mask = &mask[0];

            XISelectEvents(s->display, DefaultRootWindow(s->display), &evmask, 1);

            int devicesMax;
            XIDeviceInfo* devices = XIQueryDevice(s->display, XIAllDevices, &devicesMax);
            // NOTE: We're assuming there is only one master pointer device. This certainly SHOULD be true, AFAIK.
            for(int i = 0; i < devicesMax; i++){
                XIDeviceInfo* info = &devices[i];
                if (info->use == XIMasterPointer){
                    s->libXI_master_pointer_device = info->deviceid;
                    succeeded = true;
                    break;
                }
            }
            if(!succeeded){
                fmt_msg("XInput2 unable to find master pointer device. Unable to get raw mouse input.\n");
            }

            XIFreeDeviceInfo(devices);
        }
        else{
            fmt_msg("XIQueryVersion failed. Unable to get raw mouse input.\n");
        }
    }
    else{
        fmt_msg("XQueryExtension failed to query for the XInput extension. Unable to get raw mouse input.\n");
    }

    return succeeded;
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
            Event_Button *button = &evt->button;

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
            Event_Mouse_Motion *motion = &evt->mouse_motion;
            motion->type = Event_Type_Mouse_Motion;
            // TODO: Find out what motion hints are and if we need them.
            Xlib_Window *window = &g__display.window;

            motion->pixel_x = xevt->xmotion.x;
            motion->pixel_y = window->height - xevt->xmotion.y;
            motion->rel_x = 0;
            motion->rel_y = 0;

            s->last_mouse_x = motion->pixel_x;
            s->last_mouse_y = motion->pixel_y;
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

            if(s->window.flags & Display_Flag_HW_Rendering){
                display__create_backbuffer(&s->window);
            }
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

                case XK_space:
                    evt->key.id = Key_ID_Space; break;

                case XK_Delete:
                    evt->key.id = Key_ID_Delete; break;

                case XK_Escape:
                    evt->key.id = Key_ID_Escape; break;

                case XK_BackSpace:
                    evt->key.id = Key_ID_Backspace; break;
            }
        } break;

        case GenericEvent:{
            if(xevt->xcookie.extension == s->libXI_extension_opcode && XGetEventData(s->display, &xevt->xcookie)){
                assert(s->use_XI2_for_mouse);

                XIRawEvent* raw = (XIRawEvent*)xevt->xcookie.data;
                // Filter out events from the master pointer device because (long ago) we would never get button release from the device.
                // TODO: Is this really still an issue?
                if(raw->deviceid != s->libXI_master_pointer_device){
                    switch(xevt->xcookie.evtype){
                        case XI_RawMotion:{
                            if(raw->valuators.mask_len > 0){
                                Event_Mouse_Motion *mouse = &evt->mouse_motion;

                                mouse->type = Event_Type_Mouse_Motion;
                                // TODO: Make sure this is correct. Do the valuators always map to the x axis to 0 and the y axis to 1?
                                if (XIMaskIsSet(raw->valuators.mask, 0)){
                                    mouse->rel_x = raw->raw_values[0];
                                }
                                else{
                                    mouse->rel_x = 0;
                                }

                                if (XIMaskIsSet(raw->valuators.mask, 1)){
                                    mouse->rel_y = raw->raw_values[1];
                                }
                                else{
                                    mouse->rel_y = 0;
                                }

                                mouse->pixel_x = s->last_mouse_x;
                                mouse->pixel_y = s->last_mouse_y;
                                event_translated = true;
                            }
                        } break;

#if 0
                        // TODO: Handle mouse button presses.
                        case XI_RawButtonPress:
                        case XI_RawButtonRelease:
                        {
                            bool isDown = evt.xcookie.evtype == XI_RawButtonPress;
                            // TODO: Test to make sure this is standard! Should we allow button mapping for mice?
                            if (raw->detail == 1)
                            {
                                processButton(&mouse.buttons[MBUTTON_LEFT], isDown, time);
                            }
                            else if (raw->detail == 2)
                            {
                                processButton(&mouse.buttons[MBUTTON_MIDDLE], isDown, time);
                            }
                            else if (raw->detail == 3)
                            {
                                processButton(&mouse.buttons[MBUTTON_RIGHT], isDown, time);
                            }
                            else if (raw->detail == 4)
                            {
                                mouse.wheel = -1.0f;
                            }
                            else if (raw->detail == 5)
                            {
                                mouse.wheel = 1.0f;
                            }

                            //logInfo("Mouse detail: {0}\n", raw.detail);
                        } break;
#endif

                        default:
                        {
                            assert(!"Unknown raw mouse event type!");
                        } break;
                    }
                }
            }

            XFreeEventData(s->display, &xevt->xcookie);
        } break;
    }
    return event_translated;
}

Cbed_API Display_Backbuffer display_get_sw_backbuffer(){
    Display_Backbuffer result = {0};
    result.width  = g__display.window.width;
    result.height = g__display.window.height;
    result.pixels = g__display.window.backbuffer_pixels;
    return result;
}

Cbed_API void display_flip_backbuffer(){
    Xlib *s = &g__display;
    Xlib_Window *window = &s->window;
    if(window->flags & Display_Flag_HW_Rendering){
        glXSwapBuffers(s->display, window->handle);
    }
    else{
        XPutImage(
            s->display, s->window.handle, s->window.graphics_context,
            s->window.backbuffer,
            0, 0, 0, 0, s->window.width, s->window.height
        );
    }
}

Cbed_API bool display_next_event(Event *event){
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
    Display *display = s->display;
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

Cbed_API bool display_begin(const char *window_title, uint32_t width, uint32_t height, uint32_t window_flags){
    Xlib *s = &g__display;

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

    // The "screen" is a render target. It seems safe to use the default screen for the given display.
    int default_screen = DefaultScreen(s->display);

    XVisualInfo visual_info = {0};
    if(window_flags & Display_Flag_HW_Rendering){
        if(!display__init_glx(&visual_info, default_screen)){
            window_flags &= ~Display_Flag_HW_Rendering; // TODO: Does this actually clear the flag? Get the syntax right!
            display__get_sw_visual_info(&visual_info, default_screen);
        }
    }
    else{
        display__get_sw_visual_info(&visual_info, default_screen);
    }

    if(visual_info.visual){
        s->window = display__open_window(window_title, &visual_info, width, height, window_flags);

        if(s->window.flags & Display_Flag_HW_Rendering){
            glXMakeCurrent(s->display, s->window.handle, s->glx_context);
            load_opengl_functions((OpenGL_Load_Sym_Func)glXGetProcAddressARB);
        }
    }
    else{
        fmt_msg_puts("Unable to create window for display.\n");
    }

    s->use_XI2_for_mouse = display__request_XInput2_mouse_events();


#if 0
    XextErrorHandler default_xext_error_handler =
        XSetExtensionErrorHandler(&xext_error_handler);

    XSync(g_x11_display, False);
    XSetExtensionErrorHandler(default_xext_error_handler);

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

Cbed_API void display_end(){
    Xlib * s = &g__display;

    if(s->hidden_cursor_pixmap){
        XFreePixmap(s->display, s->hidden_cursor_pixmap);
    }
    if(s->window.flags & Display_Flag_HW_Rendering){
        XFreeColormap(s->display, s->colormap);
    }

    display__close_window(&s->window);

    //if(g_glx_context) glXDestroyContext(g_x11_display, g_glx_context);
    if(s->display){
        XCloseDisplay(s->display);
        s->display = NULL;
    }
}

Cbed_API void display_end_frame(){
    XFlush(g__display.display);
}

Cbed_API Display_Info display_get_info(){
    Xlib_Window *window = &g__display.window;
    Display_Info result = {0};
    result.window_flags  = window->flags;
    result.window_width  = window->width;
    result.window_height = window->height;
    /*fmt_msg("Window: {0},{1}\n", fmt_i(window->width), fmt_i(window->height));*/

    return result;
}

//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------
#endif
