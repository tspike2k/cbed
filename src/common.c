//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "common.h"

#include <string.h> // memcpy
#include <assert.h>

static const char *fmt__int_table = "0123456789abcdefxp";

static char fmt__memory[1024];
Fmt_Buffer  fmt__buffer;

static void        *fmt__msg_dest;
static Fmt_Put_Func fmt__msg_put;

enum{
    Fmt_Type_None,
    Fmt_Type_Unsigned_Integer,
    Fmt_Type_Signed_Integer,
    Fmt_Type_C_String,
    Fmt_Type_Float,
};

Fmt_Arg fmt_i(int64_t value){
    Fmt_Arg result = {};
    result.info = Fmt_Type_Signed_Integer;
    result.data_int = value;
    return result;
}

Fmt_Arg fmt_f(float value){
    Fmt_Arg result = {};
    result.info = Fmt_Type_Float;
    result.data_float = value;
    return result;
}

Fmt_Arg fmt_cstr(const char *value){
    Fmt_Arg result = {};
    result.info = Fmt_Type_C_String;
    result.data_cstr = value;
    return result;
}

static size_t fmt__s64(char *buffer, size_t buffer_length, int64_t value, uint32_t base){
    assert(base <= 16);

    size_t cursor = buffer_length;
    for(size_t i = cursor; i > 0; i--){
        if(value == 0) break;

        cursor = i-1;
        char c = fmt__int_table[value % base];
        buffer[cursor] = c;
        value /= base;
    }
    return cursor;
}

#include <stdio.h> // TODO: Use float conversion functiosn sourced from stb.

static void fmt__arg(Fmt_Arg arg, Fmt_Put_Func put, void *dest){
    char temp_buffer[512];
    size_t buffer_size = Array_Len(temp_buffer);
    switch(arg.info){
        default: assert(0); break;

        case Fmt_Type_Signed_Integer:{
            size_t cursor = fmt__s64(&temp_buffer[0], buffer_size, arg.data_int, 10);
            put(&temp_buffer[cursor], buffer_size - cursor, dest);
        } break;

        case Fmt_Type_Float:{
            snprintf(&temp_buffer[0], buffer_size, "%f", arg.data_float);
            put(&temp_buffer[0], strlen(&temp_buffer[0]), dest);
        } break;

        case Fmt_Type_C_String:{
            const char* s = arg.data_cstr;
            put(s, strlen(s), dest);
        } break;
    }
}

void *buffer_push_bytes(Buffer *buffer, size_t bytes){
    assert(buffer->used + bytes <= buffer->size);
    void *result = &buffer->data[buffer->used];
    memset(result, 0, bytes);
    buffer->used += bytes;
    return result;
}

void *buffer_write(Buffer *buffer, const void* data, size_t data_size){
    void *result = buffer_push_bytes(buffer, data_size);
    if(result){
        memcpy(result, data, data_size);
    }
    return result;
}

#ifdef __gnu_linux__

#include <unistd.h>

// TODO: Is there a pow function for integers?
#define fmt__pow __builtin_powif

static void fmt__default_flush(){
    if(fmt__buffer.used > 0){
        write(1, &fmt__buffer.data[0], fmt__buffer.used);
        fmt__buffer.used = 0;
    }
}

#endif // __gnu_linux__

static void fmt__default(const char *text, size_t text_count, void *dest){
    if(text){
        while(text_count > 0){
            size_t availible = fmt__buffer.size - fmt__buffer.used;
            size_t to_write  = text_count < availible ? text_count : availible;

            memcpy(&fmt__buffer.data[fmt__buffer.used], text, to_write);
            fmt__buffer.used += to_write;
            if(fmt__buffer.used == fmt__buffer.size || text[text_count-1] == '\n'){
                fmt__default_flush();
            }

            text_count -= to_write;
            text += to_write;
        }
    }
    else{
        fmt__default_flush();
    }
}

void ceabed_begin(){
    fmt__buffer = fmt_make_buffer(&fmt__memory[0], Array_Len(fmt__memory));
    fmt__msg_dest = &fmt__buffer;
    fmt__msg_put  = fmt__default;
}

void ceabed_end(){
    fmt__msg_put(NULL, 0, fmt__msg_dest); // Flush the buffer.
}

void fmt_msg_set_dest(Fmt_Put_Func put, void *user_data){
    fmt__msg_dest = user_data;
    fmt__msg_put = put;
}

void fmt_msg_put(const char* msg, size_t msg_length){
    fmt__msg_put(msg, msg_length, fmt__msg_dest);
}

void fmt_msg_puts(const char* msg){
    fmt__msg_put(msg, strlen(msg), fmt__msg_dest);
}

Fmt_Buffer fmt_make_buffer(char *buffer, size_t length){
    Fmt_Buffer result = {buffer, length};
    return result;
}

static void fmt__buffer_put(const char* text, size_t text_count, void *dest){
    if(text_count){
        Fmt_Buffer *buffer = (Fmt_Buffer*)dest;
        size_t availible = buffer->size - buffer->used;
        size_t to_copy = text_count < availible ? text_count : availible;
        memcpy(&buffer->data[buffer->used], text, to_copy);
        buffer->used += to_copy;
    }
}

static size_t fmt__read_until(const char* fmt_string, size_t *reader, char delimiter_a, char delimiter_b){
    size_t start = *reader;
    char c = fmt_string[*reader];
    while(c != '\0' && c != delimiter_a && c != delimiter_b){
        (*reader)++;
        c = fmt_string[*reader];
    }
    return start;
}

static bool fmt__parse_spec(const char *fmt_string, size_t *reader, size_t *arg_index){
    assert(fmt_string[*reader] == '{');
    (*reader)++;
    size_t start = fmt__read_until(fmt_string, reader, '}', '\0');
    size_t len = *reader - start;
    if(fmt_string[*reader] == '}')
        (*reader)++;

    bool success = len > 0;

    size_t result = 0;
    for(size_t i = len; i > 0; i--){
        char c = fmt_string[start + i-1];
        if(c >= '0' && c <= '9'){
            result += (c - '0') * fmt__pow(10, len-i);
        }
        else{
            success = false;
            break;
        }
    }

    *arg_index = result;
    return success;
}

void fmt_buffer_raw(const char *fmt_string, Fmt_Buffer *dest, Fmt_Arg* args, size_t args_count){
    size_t reader = 0;
    while(fmt_string[reader] != '\0'){
        size_t start = fmt__read_until(fmt_string, &reader, '{', '\0');
        fmt__buffer_put(&fmt_string[start], reader - start, dest);

        if(fmt_string[reader] == '{'){
            size_t arg_index;
            if(fmt__parse_spec(fmt_string, &reader, &arg_index)
            && arg_index < args_count){
                fmt__arg(args[arg_index], fmt__buffer_put, dest);
            }
        }
    }
    // Null terminate the end of the resulting string.
    size_t term_index = dest->used < dest->size ? dest->used : dest->size;
    dest->data[term_index] = '\0';
}

void fmt_msg_raw(const char *fmt_string, Fmt_Arg *args, size_t args_count){
    size_t reader = 0;
    while(fmt_string[reader] != '\0'){
        size_t start = fmt__read_until(fmt_string, &reader, '{', '\n');

        fmt__msg_put(&fmt_string[start], reader - start, fmt__msg_dest);
        if(fmt_string[reader] == '{'){
            size_t arg_index;
            if(fmt__parse_spec(fmt_string, &reader, &arg_index)
            && arg_index < args_count){
                fmt__arg(args[arg_index], fmt__msg_put, fmt__msg_dest);
            }
        }
        else if(fmt_string[reader] == '\n'){
            fmt__msg_put("\n", 1, fmt__msg_dest);
            fmt__msg_put(NULL, 0, fmt__msg_dest);
            reader++;
        }
    }
}
