//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------
#ifndef CEABED_MEMORY_H
#define CEABED_MEMORY_H

#include "cyu.h"

void *memset(void* s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
size_t strlen(const char *s);

void *alloc_memory(size_t size, uint32_t flags);
void  dealloc_memory(void *memory, size_t size);

typedef struct Slice{
    char*  text;
    size_t length;
} String;

typedef struct{
    uint8_t* data;
    size_t   length;
} Buffer;


#if 0
Memory_Block make_memory_block(void *memory, size_t size);
void        *memory_alloc(Memory_Block *block, size_t size, uint32_t flags, size_t align);
void         memory_drop(Memory_Block *block);
void         memory_push_frame(Memory_Block *block);
void         memory_pop_frame(Memory_Block *block);
void         memory_write(Memory_Block *block, void *data, size_t size);
void         memory_write_str(Memory_Block *block, char *text, size_t size);
void         memory_terminate_str(Memory_Block *block);
void        *memory_read(Memory_Block* block, size_t size);

//
// Slices
//

bool  is_char_whitespace(char c);
Slice slice(void *data, size_t length);
Slice slice_cstr(const char *data);
Slice slice_and_advance(Slice *slice, size_t size);
void  slice_advance(Slice* slice, size_t size);
void  slice_advance_char(Slice* slice); // TODO: Remove for being redundant?
bool  slice_eat_line(Slice* reader, Slice* slice);
bool  slice_eat_until_char(Slice* reader, char c);
void  slice_skip_whitespace(Slice *slice);
bool  slice_eat_next_word(Slice *reader, Slice *word);
bool  slice_matches_cstr(Slice slice, const char* str);
bool  slice_begins_with(Slice a, Slice b);
void  slice_to_cstr(Slice* s, char *buffer, size_t buffer_size);
char *slice_get_last_char(Slice s, char c);

// TODO: Remove these in favor of using Memory_Blocks instead?
Cyu_API void slice_write(Slice *writer, const void *data, size_t length);
Cyu_API void slice_write_str(Slice* writer, char* text, size_t length);
Cyu_API void slice_terminate_str(Slice* writer);

//
// Memory Blocks
//

#define Default_Align sizeof(uint32_t)

typedef struct Memory_Block_Frame Memory_Block_Frame;

struct Memory_Block_Frame {
    Memory_Block_Frame *next;
    size_t              used;
};

// TODO: We could switch from calling these Memory_Blocks back to Allocators. Then each Allocator could have a pointer to a scratch buffer.
// This way we wouldn't have to pass a scratch buffer to every single function that needs temporary scratch memory. I like this idea.
// What would make this different than our last allocator strategy is that we wouldn't be doing any sort of C-style subclassing; no casting the
// base type when passing allocators. This is much simpler, which I like. If we need subclassing, use a union and a type ID. That would be MUCH
// simpler.

typedef struct Memory_Block{
    uint8_t *base;
    size_t   used;
    size_t   size;
    Memory_Block_Frame *last_frame;
} Memory_Block;

#define memory_alloc_type(block, T, flags, align) (T *)memory_alloc(block, sizeof(T), (flags), (align));
#define memory_alloc_array(block, T, count, flags, align) (T *)memory_alloc(block, sizeof(T)*(count), (flags), (align));
#define memory_read_type(block, T) (T*)memory_read(block, sizeof(T))

Memory_Block make_memory_block(void* memory, size_t size);
void        *memory_alloc(Memory_Block *block, size_t size, uint32_t flags, size_t align);
void         memory_drop(Memory_Block *block);
void         memory_push_frame(Memory_Block *block);
void         memory_pop_frame(Memory_Block *block);
void         memory_write(Memory_Block *block, void *data, size_t size);
void         memory_write_str(Memory_Block *block, char *text, size_t size);
void         memory_terminate_str(Memory_Block *block);
void        *memory_read(Memory_Block* block, size_t size);

// TODO: Should we just use varargs? We'd lose type safety, though.
#define combine_strings(memory, result, ...){                            \
    Slice cyu__params[] = {__VA_ARGS__};                                 \
    combine_strings_raw(memory, result, &cyu__params[0], Array_Length(cyu__params)); \
}

void combine_strings_raw(Memory_Block *block, Slice* result, Slice *strings, size_t strings_count);

#endif

#endif // CEABED_MEMORY_H
