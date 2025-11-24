//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_COMMON_H
#define CEABED_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//
// Common types
//
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

//
// OS compile-time constants
//
#ifdef __gnu_linux__
#define OS_Linux
#endif

//
// Common functions
//

void ceabed_begin();
void ceabed_end();

//
// Buffers
//

typedef struct {
    u8*    data;
    size_t size;
    size_t used;
} Buffer;

#define buffer_push_array(T, buffer, count) (T*)buffer_push_bytes(buffer, sizeof(T)*(count))
#define buffer_push_type(T, buffer) (T*)buffer_push_bytes(buffer, sizeof(T))
void *buffer_push_bytes(Buffer *buffer, size_t bytes);

#define buffer_write_text(buffer, text, text_size) (char*)buffer_write(buffer, text, text_size)
#define buffer_write_type(buffer, t) buffer_write(buffer, t, sizeof(*t))
void *buffer_write(Buffer *buffer, const void* data, size_t data_size);

//
// Scratch memory
//

#if 0
#define Scratch_Memory_Size (8*1024*1024)

extern Buffer scratch_memory;
void          scratch_push_frame();
void          scratch_pop_frame();
#endif

//
// String formatting functions
//

typedef struct{
    uint32_t info; // TODO: This should contain both the arg type and an array count. Array count is limited!
    union{
        int64_t     data_int;
        uint64_t    data_uint;
        const char *data_cstr;
        void       *data_pointer;
    };
} Fmt_Arg;

// NOTE: The macros fmt_buffer and fmt_out both expand to a scoped code block. As such,
// fmt_buffer can't return a value like a normal function. But the caller needs to know
// how much text was written into the buffer. To simplify that issue, we provide the
// Fmt_Buffer data type which is passed by pointer to the fmt_buffer macro.
typedef struct{
    char*  data;
    size_t size;
    size_t used;
} Fmt_Buffer;

// NOTE: This callback is used internally by the fmt_msg_* functions to copy text to some sort
// of destination. The default destination is buffered output to stdout. This can be changed
// using the fmt_msg_set_dest function. If the function is called passing NULL as the text
// parameter this indicates a flush request. If the destination is some form of buffered ouput
// this case should be handled. Internally, ceabed_end() should be called before a program exits,
// which will trigger a flush request with this callback.
typedef void (*Fmt_Put_Func)(const char *text, size_t text_count, void *user_data);

#define Array_Len(a) (sizeof((a)) / sizeof((a)[0]))

// NOTE: If zero arguments are passed as varargs then fmt_args will still have a length of 1.
// This shouldn't cause much of an issue as that bug can be pretty easily found, but it is a
// limitation of this approach.
#define fmt_msg(s, ...){                                        \
    Fmt_Arg fmt__args[] = {__VA_ARGS__};                        \
    fmt_msg_raw((s), &fmt__args[0], Array_Len(fmt__args)); \
}

#define fmt_buffer(s, dest, ...){                                        \
    Fmt_Arg fmt__args[] = {__VA_ARGS__};                                 \
    fmt_buffer_raw((s), dest, &fmt__args[0], Array_Len(fmt__args)); \
}

Fmt_Arg fmt_i(int64_t value);
Fmt_Arg fmt_cstr(const char *s);

void fmt_msg_set_dest(Fmt_Put_Func put, void *user_data);
void fmt_msg_put(const char* msg, size_t msg_length);
void fmt_msg_puts(const char* msg);
void fmt_msg_raw(const char *fmt_string, Fmt_Arg *args, size_t args_count);

Fmt_Buffer fmt_make_buffer(char *buffer, size_t length);
void fmt_buffer_raw(const char *fmt_string, Fmt_Buffer *buffer, Fmt_Arg *args, size_t args_count);

#endif // CEABED_COMMON_H
