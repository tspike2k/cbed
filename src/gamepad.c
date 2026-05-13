//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "gamepad.h"

const char *Gamepad__Button_Names[Gamepad_Button_Max] = {
    "Gamepad_Button_Unkown",
    "Gamepad_Button_Left",
    "Gamepad_Button_Right",
    "Gamepad_Button_Up",
    "Gamepad_Button_Down",
    "Gamepad_Button_A",
    "Gamepad_Button_B",
    "Gamepad_Button_X",
    "Gamepad_Button_Y",
    "Gamepad_Button_L1",
    "Gamepad_Button_R1",
};

const char *Gamepad__Stick_Names[Gamepad_Button_Max] = {
    "Gamepad_Stick_Unknown",

    "Gamepad_Stick_LX",
    "Gamepad_Stick_LY",
    "Gamepad_Stick_RX",
    "Gamepad_Stick_RY",
};

Ceabed_API String gamepad_get_input_event_string(Gamepad_Event evt){
    String result;
    switch(evt.type){
        default: result = str_lit("Not Input");

        case Gamepad_Event_Button: result = str(Gamepad__Button_Names[evt.id]); break;
        case Gamepad_Event_Stick:  result = str(Gamepad__Stick_Names[evt.id]); break;
    }
    return result;
}

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
#include <stdlib.h> // malloc, realloc, free
#include <poll.h>

typedef struct{
    int fd;
    int rw_status;
} Gamepad;

static int      gamepad__inotify_fd;
static int      gamepad__input_watch_fd;
static u32      gamepad__capacity;
static u32      gamepad__count;
static Gamepad *gamepad__devices;

static bool gamepad__maybe_add_by_path(const char *path){
    bool result = false;

    int fd = open(path, O_RDWR|O_NONBLOCK, 0); // TODO: Should we fall back to read-only if we can't get RW access?
    if(fd != -1){
        // TODO: Check to make sure the opened FD is, in fact, a gamepad. This can be done with
        // esoteric system calls.
        if(gamepad__count >= gamepad__capacity){
            gamepad__capacity = MAX(gamepad__capacity * 2, 8);
            gamepad__devices = (Gamepad *)realloc(gamepad__devices, gamepad__capacity*sizeof(Gamepad));
        }

        Gamepad *g = &gamepad__devices[gamepad__count++];
        memset(g, 0, sizeof(Gamepad));
        g->fd = fd;

        result = true;
    }
    else{
        fmt_msg("Error: Unable to open gamepad at path {0}\n", fmt_cstr(path));
    }

    return result;
}

Ceabed_API bool gamepad_begin(const char *bindings_file_path, Buffer *temp){
    gamepad__inotify_fd = inotify_init1(IN_NONBLOCK);
    if(gamepad__inotify_fd == -1){
        fmt_msg_puts("Failed to create inotify fd for gamepad.\n");
        return false;
    }

    u32 target_events = IN_CREATE|IN_DELETE;
    int gamepad__input_watch_fd = inotify_add_watch(gamepad__inotify_fd, "/dev/input", target_events);
    if(gamepad__input_watch_fd == -1){
        fmt_msg_puts("Failed to setup watch fd on /dev/input for gamepads.\n");
        return false;
    }

    DIR *dir = opendir("/dev/input/by-id");
    if(dir){
        for(struct dirent *next = readdir(dir); next; next = readdir(dir)){
            if(next->d_type == DT_LNK
            && str_ends_with(str(next->d_name), str_lit("-event-joystick"))){
                /*fmt_msg("Found joystick: {0}\n", fmt_cstr(next->d_name));*/
                size_t marker = buffer_frame_begin(temp);
                String path = fmt_buffer("/dev/input/by-id/{0}", temp, fmt_cstr(next->d_name));
                gamepad__maybe_add_by_path(path.text);
                buffer_frame_end(temp, marker);
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
}

Ceabed_API void gamepad_update(Buffer *temp){
    if(gamepad__count == 0) return;

    size_t marker = buffer_frame_begin(temp);
    struct pollfd *poll_fds = buffer_push_array(struct pollfd, temp, gamepad__count);
    for_count(size_t, i, gamepad__count){
        struct pollfd *entry = &poll_fds[i];
        entry->events = POLLIN|POLLOUT;
        entry->fd = gamepad__devices[i].fd;
    }

    // TODO: Use epoll instead? It's supposed to be faster.
    poll(poll_fds, gamepad__count, 0);
    for_count(size_t, i, gamepad__count){
        gamepad__devices[i].rw_status = poll_fds[i].revents;
    }

    // TODO: Check to see if we need to add gamepads using inotify.
    buffer_frame_end(temp, marker);
}

Ceabed_API bool gamepad_poll(u32 gamepad_index, Gamepad_Event *event){
    assert(gamepad_index < gamepad__count);

    bool result = false;

    Gamepad *pad = &gamepad__devices[gamepad_index];
    if(pad->rw_status & POLLIN){ // We can read without blocking
        struct input_event e;

        ssize_t r = read(pad->fd, &e, sizeof(e));
        if(r < 0){
            // TODO: Handle errors
        }
        else{
            // TODO: Mapping of event codes to correct buttons/axis!

            switch(e.type){
                default: break;

                case EV_KEY:{
                    event->type = Gamepad_Event_Button;
                    switch(e.code){
                        default: event->id = Gamepad_Button_Unknown; break;

                        case BTN_WEST:  event->id = Gamepad_Button_Y; break;
                        case BTN_NORTH: event->id = Gamepad_Button_X; break;
                        case BTN_EAST:  event->id = Gamepad_Button_B; break;
                        case BTN_SOUTH: event->id = Gamepad_Button_A; break;
                    }

                    event->value = e.value; // TODO: Do we want a boolean here? Pressed or released?
                    result = true;
                } break;

                case EV_ABS:{
                    // TODO: Handle joysticks
                    event->type = Gamepad_Event_Stick;
                    switch(e.code){
                        default: event->id = Gamepad_Stick_Unknown; break;

                        case ABS_X:  event->id = Gamepad_Stick_LX; break;
                        case ABS_Y:  event->id = Gamepad_Stick_LY; break;
                        case ABS_RX: event->id = Gamepad_Stick_RX; break;
                        case ABS_RY: event->id = Gamepad_Stick_RY; break;
                    };

                    event->value = e.value; // TODO: Convert between the range of -1 and 1
                    result = true;
                } break;
            }
        }
    }

    return result;
}

Ceabed_API u32  gamepad_get_count(){
    u32 result = gamepad__count;
    return result;
}

#endif
//------------------------------------------------------------------------------
// OS_Linux
//------------------------------------------------------------------------------
