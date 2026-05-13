//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
TODO:
    We should be able to do automatic controller mapping using SDLs massive gamepad control map. Events automatically use the mapping.
*/

#include "common.h"

typedef struct {

} Gamepad_Event;

Ceabed_API bool gamepad_begin(const char *bindings_file_path, Buffer *temp);
Ceabed_API void gamepad_end();
Ceabed_API void gamepad_update(Buffer *temp);
Ceabed_API bool gamepad_poll(u32 gamepad_index, Gamepad_Event *event);
Ceabed_API u32  gamepad_get_count();
