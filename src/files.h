//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CEABED_FILES_H
#define CEABED_FILES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

enum{
    File_Flag_Is_Open        = (1 << 0),
    File_Flag_Read           = (1 << 1),
    File_Flag_Write          = (1 << 2),
    File_Flag_No_Trunc       = (1 << 3),
    File_Flag_No_Open_Errors = (1 << 5),
    File_Flag_Error          = (1 << 6),
};

typedef struct{
    uint32_t flags;
    uint64_t handle;
    size_t   cursor;
} File;

// NOTE: Read/write operations are done in a blocking mode.
Ceabed_API bool file_open(File *file, const char *file_path, uint32_t flags);
Ceabed_API void file_close(File *file);
Ceabed_API size_t file_read(File *file, size_t offset, void *buffer, size_t buffer_size);
Ceabed_API void file_write(File *file, size_t offset, void *buffer, size_t buffer_size);
Ceabed_API size_t file_get_size(File* file);
Ceabed_API void file_delete(const char *file_path);
Ceabed_API size_t file_stream_in(File *file, void *buffer, size_t buffer_size);
Ceabed_API void file_stream_out(File *file, void *buffer, size_t buffer_size);
Ceabed_API bool file_exists(const char *file_path);

Ceabed_API void file_write_from_memory(const char *file_name, void *data, size_t size);
Ceabed_API String file_read_into_memory(const char *file_name, Buffer *buffer);

Ceabed_API File file_get_stdin();
Ceabed_API File file_get_stdout();
Ceabed_API File file_get_stderr();

// TODO: Implement directory recursion
#if 0
typedef struct{
    char internal[32];
} File_Walker;

enum File_Type: uint{
    None,
    File,
    Directory,
    Unknown,
}

struct File_Entry{
    File_Type   type;
    const char* name;
}

File_Walker file_recurse_dir(const char *dir, void *memory, size_t memory_size);
File_Entry file_recurse_next(File_Walker* walker);

#endif

#endif // CEABED_FILES_H
