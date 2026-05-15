//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "gamepad.h"

String gamepad__input_names[Gamepad_Input_Max] = {
    str_lit("Unknown"),

    str_lit("Button Left"),
    str_lit("Button Right"),
    str_lit("Button Up"),
    str_lit("Button Down"),
    str_lit("Button A"),
    str_lit("Button B"),
    str_lit("Button X"),
    str_lit("Button Y"),
    str_lit("Button L1"),
    str_lit("Button L2"),
    str_lit("Button L3"),
    str_lit("Button R1"),
    str_lit("Button R2"),
    str_lit("Button R3"),
    str_lit("Button Start"),
    str_lit("Button Select"),

    str_lit("Axis LX"),
    str_lit("Axis LY"),
    str_lit("Axis RX"),
    str_lit("Axis RY"),
    str_lit("Axis Dir X"),
    str_lit("Axis Dir Y"),
};

//------------------------------------------------------------------------------
// OS_Linux
//------------------------------------------------------------------------------
#ifdef OS_Linux

#include <linux/input.h> // input_event
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h> // realloc, free, strdup
#include <poll.h>

#define Gamepad__Base_Path "/dev/input/by-id"

typedef struct{
    int fd;
    int rw_status;
    char *file_name;
} Gamepad;

static int      gamepad__inotify_fd;
static int      gamepad__input_watch_fd;
static u32      gamepad__capacity;
static u32      gamepad__count;
static Gamepad *gamepad__devices;

static bool gamepad__maybe_add_by_path(const char *file_name, Buffer *temp){
    size_t marker = buffer_frame_begin(temp);
    bool result = false;

    String path = fmt_buffer(Gamepad__Base_Path "/{0}", temp, fmt_cstr(file_name));
    int fd = open(path.text, O_RDWR|O_NONBLOCK, 0); // TODO: Should we fall back to read-only if we can't get RW access?
    if(fd != -1){
        // TODO: Check to make sure the opened FD is, in fact, a gamepad. This can be done with
        // esoteric system calls.
        if(gamepad__count >= gamepad__capacity){
            gamepad__capacity = MAX(gamepad__capacity * 2, 8);
            gamepad__devices = (Gamepad *)realloc(gamepad__devices, gamepad__capacity*sizeof(Gamepad));
        }

        Gamepad *pad = &gamepad__devices[gamepad__count++];
        memset(pad, 0, sizeof(Gamepad));
        pad->fd = fd;
        pad->file_name = strdup(file_name);

        result = true;
    }
    else{
        fmt_msg("Error: Unable to open gamepad at path {0}\n", fmt_cstr(path.text));
    }

    buffer_frame_end(temp, marker);
    return result;
}

Ceabed_API bool gamepad_begin(const char *bindings_file_path, Buffer *temp){
    gamepad__inotify_fd = inotify_init1(IN_NONBLOCK);
    if(gamepad__inotify_fd == -1){
        fmt_msg_puts("Failed to create inotify fd for gamepad.\n");
        return false;
    }

    u32 target_events = IN_CREATE|IN_DELETE;
    int gamepad__input_watch_fd = inotify_add_watch(gamepad__inotify_fd, Gamepad__Base_Path, target_events);
    if(gamepad__input_watch_fd == -1){
        fmt_msg_puts("Failed to setup watch fd on /dev/input for gamepads.\n");
        return false;
    }

    DIR *dir = opendir(Gamepad__Base_Path);
    if(dir){
        for(struct dirent *next = readdir(dir); next; next = readdir(dir)){
            if(next->d_type == DT_LNK
            && str_ends_with(str(next->d_name), str_lit("-event-joystick"))){
                gamepad__maybe_add_by_path(next->d_name, temp);
            }
        }
        closedir(dir);
    }
    else{
        // NOTE: This isn't really a critical error, as we were able to watch the directory if
        // we got this far. It would mean the user would have to re-connect any gamepads the
        // player wishes to use.
        fmt_msg_puts("Failed to open /dev/input/by-id to scan for connected gamepads. Please reconnect any gamepads you wish to use.\n");
    }

    return true;
}

Ceabed_API void gamepad_end(){
    if(gamepad__inotify_fd != -1){
        if(gamepad__input_watch_fd != -1){
            inotify_rm_watch(gamepad__inotify_fd, gamepad__input_watch_fd);
            gamepad__input_watch_fd = -1;
        }

        close(gamepad__inotify_fd);
        gamepad__inotify_fd = -1;
    }

    for_count(u32, i, gamepad__count){
        Gamepad *pad = &gamepad__devices[i];
        if(pad->fd != -1){
            close(pad->fd);
        }
        free(pad->file_name);
    }
    free(gamepad__devices);
}

Ceabed_API void gamepad_update(Buffer *temp){
    if(gamepad__count == 0) return;

    size_t marker = buffer_frame_begin(temp);
    struct pollfd *poll_fds = buffer_push_array(struct pollfd, temp, gamepad__count);
    for_count(u32, i, gamepad__count){
        struct pollfd *entry = &poll_fds[i];
        entry->events = POLLIN|POLLOUT;
        entry->fd = gamepad__devices[i].fd;
    }

    // TODO: Use epoll instead? It's supposed to be faster.
    poll(poll_fds, gamepad__count, 0);
    for_count(u32, i, gamepad__count){
        gamepad__devices[i].rw_status = poll_fds[i].revents;
    }

    // TODO: Check to see if we need to add gamepads using inotify.
    buffer_frame_end(temp, marker);
}

static bool gamepad__translate_event(Gamepad_Event *dest, struct input_event source){
    // TODO: Handle custom key bindings here.

    dest->internal_type = source.type;
    dest->internal_id   = source.code;
    dest->value = source.value;

    bool translated = false;
    switch(source.type){
        default: break;

        case EV_KEY:{
            switch(source.code){
                default: dest->id = Gamepad_Input_Unknown; break;

                case BTN_WEST:   dest->id = Gamepad_Button_Y; break;
                case BTN_NORTH:  dest->id = Gamepad_Button_X; break;
                case BTN_EAST:   dest->id = Gamepad_Button_B; break;
                case BTN_SOUTH:  dest->id = Gamepad_Button_A; break;
                case BTN_TL:     dest->id = Gamepad_Button_L1; break;
                case BTN_TR:     dest->id = Gamepad_Button_R1; break;
                case BTN_SELECT: dest->id = Gamepad_Button_Select; break;
                case BTN_START:  dest->id = Gamepad_Button_Start; break;
                case BTN_THUMBL: dest->id = Gamepad_Button_L3; break;
                case BTN_THUMBR: dest->id = Gamepad_Button_R3; break;
            }

            dest->type = Gamepad_Event_Input;
            translated = true;
        } break;

        case EV_ABS:{
            switch(source.code){
                default: dest->id = Gamepad_Input_Unknown; break;

                case ABS_HAT0X: dest->id = Gamepad_Axis_Dir_X; break;
                case ABS_HAT0Y: {
                    dest->id = Gamepad_Axis_Dir_Y;
                    dest->value = -dest->value; // Make positive Y upwards
                } break;
                case ABS_X:     dest->id = Gamepad_Axis_LX; break;
                case ABS_Y:     dest->id = Gamepad_Axis_LY; break;
                case ABS_RX:    dest->id = Gamepad_Axis_RX; break;
                case ABS_RY:    dest->id = Gamepad_Axis_RY; break;
                case ABS_Z:     dest->id = Gamepad_Button_L2; break;
                case ABS_RZ:    dest->id = Gamepad_Button_R2; break;
            };

            dest->type = Gamepad_Event_Input;
            translated = true;
        } break;
    }

    return translated;
}

Ceabed_API bool gamepad_next_event(u32 gamepad_index, Gamepad_Event *event){
    assert(gamepad_index < gamepad__count);

    bool result = false;
    Gamepad *pad = &gamepad__devices[gamepad_index];
    if(pad->rw_status & POLLIN){ // We can read without blocking
        // Loop to handle short reads and skip events that can't be translated
        while(true){
            struct input_event e;
            ssize_t r = read(pad->fd, &e, sizeof(e));
            if(r == sizeof(e)){
                if(gamepad__translate_event(event, e)){
                    result = true;
                    break;
                }
            }
            else{
                // NOTE: Even though we use poll to see if the file descriptor can be read from,
                // it's still possible there may be nothing to read. In that case, read() will
                // return EAGAIN since we opened the fd in non-blocking mode.
                if(errno != EAGAIN){
                    const char *error_msg = strerror(errno);
                    fmt_msg("Got error '{0}' when reading gamepad {1}\n", fmt_cstr(error_msg), fmt_cstr(pad->file_name));
                }
                else{
                    break;
                }
            }
        }
    }

    return result;
}

Ceabed_API u32  gamepad_get_count(){
    u32 result = gamepad__count;
    return result;
}

static String gamepad__get_unknown_event_string(Gamepad_Event event, Buffer *buffer){
    String result;
    switch(event.internal_type){
        default:     result = str_lit("Unknown"); break;
        case EV_KEY: result = fmt_buffer("Button {0}", buffer, fmt_i(event.internal_id)); break;
        case EV_ABS: result = fmt_buffer("Axis {0}", buffer, fmt_i(event.internal_id)); break;
        case EV_SYN: result = str_lit("EV_SYN"); break;
    }

    return result;
}

#endif
//------------------------------------------------------------------------------
// OS_Linux
//------------------------------------------------------------------------------

Ceabed_API String gamepad_get_event_string(Gamepad_Event event, Buffer *buffer){
    String result;
    switch(event.type){
        default: assert(0);

        case Gamepad_Event_Unknown:    result = gamepad__get_unknown_event_string(event, buffer); break;
        case Gamepad_Event_Connect:    result = str_lit("Connect"); break;
        case Gamepad_Event_Disconnect: result = str_lit("Disconnect"); break;

        case Gamepad_Event_Input:{
            if(event.id != Gamepad_Input_Unknown){
                assert(event.id < Gamepad_Input_Max);
                result = gamepad__input_names[event.id];
            }
            else{
                result = gamepad__get_unknown_event_string(event, buffer);
            }
        } break;
    }

    return result;
}
