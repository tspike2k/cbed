//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "files.h"
#include "common.h"
#include <assert.h>

//------------------------------------------------------------------------------
// OS_Linux
//------------------------------------------------------------------------------
#ifdef OS_Linux

#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h> // malloc, realloc, free

Ceabed_API bool file_open(File *file, const char *file_path, uint32_t flags){
    uint32_t default_file_permissions = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;

    int permissions = 0;
    int oflags = 0;
    if((flags & File_Flag_Read) && (flags & File_Flag_Write)){
        oflags = O_RDWR|O_CREAT;
    }
    else if(flags & File_Flag_Read){
        oflags = O_RDONLY;
    }
    else if(flags & File_Flag_Write){
        oflags = O_WRONLY|O_CREAT;
        permissions = default_file_permissions;
    }

    if((flags & File_Flag_Write) && !(flags & File_Flag_No_Trunc)){
        oflags |= O_TRUNC;
    }

    bool result = false;
    int fd = open(file_path, oflags, permissions);
    if(fd != -1){
        result = true;
        file->handle = fd;
        file->flags = flags|File_Flag_Is_Open;
    }
    else if(!(flags & File_Flag_No_Open_Errors)){
        const char *error_msg = strerror(errno);
        fmt_msg("Unable to open {0}: {1}\n", fmt_cstr(file_path), fmt_cstr(error_msg));
    }
    return result;
}

Ceabed_API size_t file_read(File *file, size_t offset, void *buffer, size_t buffer_size){
    assert(file->flags & File_Flag_Read);

    int fd = (int)file->handle;
    uint8_t *dest = (uint8_t*)buffer;
    size_t bytes_read = 0;
    while(bytes_read < buffer_size){
        ssize_t r = pread(fd, &dest[bytes_read], buffer_size - bytes_read, offset + bytes_read);
        if(r == -1){
            if(errno == -EINTR){
                // Reading was interupted by a signal. Do nothing to try again.
                continue;
            }
            else{
                const char *error_msg = strerror(errno);
                fmt_msg("Failed to read from file: {0}\n", fmt_cstr(error_msg));
                file->flags |= File_Flag_Error;
            }
        }
        else{
            bytes_read += (size_t)r;
        }
        break;
    }

    file->cursor = offset + bytes_read;
    return bytes_read;
}

Ceabed_API void file_write(File *file, size_t offset, void *buffer, size_t buffer_size){
    assert(file->flags & File_Flag_Write);

    int fd = (int)file->handle;
    uint8_t *dest = (uint8_t*)buffer;
    size_t buffer_written = 0;
    while(buffer_written < buffer_size){
        ssize_t r = pwrite(fd, &dest[buffer_written], buffer_size - buffer_written, offset + buffer_written);
        if(r == -1){
            if(errno == -EINTR){
                // Writing was interupted by a signal. Do nothing to try again.
                continue;
            }
            else{
                const char *error_msg = strerror(errno);
                fmt_msg("Failed to write to file: {0}\n", fmt_cstr(error_msg));
                file->flags |= File_Flag_Error;
            }
        }
        else{
            buffer_written += (size_t)r;
        }
        break;
    }

    file->cursor = offset + buffer_written;
}

Ceabed_API size_t file_stream_in(File *file, void *buffer, size_t buffer_size){
    assert(file->flags & File_Flag_Read);

    int fd = (int)file->handle;
    uint8_t *dest = (uint8_t*)buffer;
    size_t bytes_read = 0;
    while(bytes_read < buffer_size){
        ssize_t r = read(fd, &dest[bytes_read], buffer_size - bytes_read);
        if(r == -1){
            if(errno == -EINTR){
                // Reading was interupted by a signal. Do nothing to try again.
                // TODO: Log the error.
                // log_error("Failed to read from file: {0}\n", strerror(errno));
                continue;
            }
            else{
                file->flags |= File_Flag_Error;
            }
        }
        else{
            bytes_read += (size_t)r;
        }
        break;
    }

    return bytes_read;
}

Ceabed_API void file_stream_out(File *file, void *buffer, size_t buffer_size){
    assert(file->flags & File_Flag_Write);

    int fd = (int)file->handle;
    uint8_t *dest = (uint8_t*)buffer;
    size_t buffer_written = 0;
    while(buffer_written < buffer_size){
        ssize_t r = write(fd, &dest[buffer_written], buffer_size - buffer_written);
        if(r == -1){
            if(errno == -EINTR){
                // Writing was interupted by a signal. Do nothing to try again.
                // TODO: Log the error.
                // log_error("Failed to read from file: {0}\n", strerror(errno));
                continue;
            }
            else{
                file->flags |= File_Flag_Error;
            }
        }
        else{
            buffer_written += (size_t)r;
        }
        break;
    }
}

Ceabed_API void file_close(File* file){
    assert(file->flags & File_Flag_Is_Open);
    int fd = (int)file->handle;
    close(fd);
    file->flags = 0;
}

Ceabed_API size_t file_get_size(File* file){
    int fd = (int)file->handle;
    struct stat s;
    size_t result;
    if(fstat(fd, &s) == 0){
        result = s.st_size;
    }
    else{
        /*log("Unable to get file size.\n");*/
    }
    return result;
}

Ceabed_API void file_write_from_memory(const char *file_name, void *data, size_t size){
    File file;
    if(file_open(&file, file_name, File_Flag_Write)){
        file_write(&file, 0, data, size);
        file_close(&file);
    }
}

Ceabed_API String file_read_into_memory(const char *file_name, Buffer *buffer){
    File file;
    String result = {};
    if(file_open(&file, file_name, File_Flag_Read)){
        size_t size = file_get_size(&file);
        char *contents = (char*)buffer_push_bytes(buffer, size+1);
        file_read(&file, 0, contents, size);
        contents[size] = '\0';
        result = (String){contents, size};
        file_close(&file);
    }
    return result;
}

Ceabed_API File file_get_stdin(){
    uint32_t flags = File_Flag_Is_Open|File_Flag_Read;
    File result = {flags, 0};
    return result;
}

Ceabed_API File file_get_stdout(){
    uint32_t flags = File_Flag_Is_Open|File_Flag_Write;
    File result = {flags, 1};
    return result;
}

Ceabed_API File file_get_stderr(){
    uint32_t flags = File_Flag_Is_Open|File_Flag_Write;
    File result = {flags, 2};
    return result;
}

Ceabed_API const char *get_executable_path(Buffer *buffer){
    // TODO: Make sure the pointer is word aligned.
    const char *result = "./";
    ssize_t count = readlink("/proc/self/exe", (char*)buffer->data, buffer->size - buffer->used);
    if(count > 0){
        // Remove the trailing binary name from the result
        char *scanner = (char*)buffer->data;
        char *place   = NULL;
        while(*scanner != '\0'){
            if(*scanner == '/'){
                place = scanner+1;
            }
            scanner++;
        }

        size_t length = scanner - (char*)buffer->data;
        if(place){
            *place = '\0';
            length = place - (char*)buffer->data;
        }

        result = (char*)buffer->data;
        buffer->used += length+1;
    }
    else{
        fmt_msg("Unable to get executable path. Falling back to relative path.\n");
    }
    return result;
}

// TODO: Shouldn't this be called file_delete?
Ceabed_API void delete_file(const char *file_path){
    // TODO: Error handling?
    unlink(file_path);
}

// Due to various oddities with POSIX, the most reliable way to test if a file exists is to simply
// open the file. Both access and stat can give erronous results if the application is being
// run with the SUID bit set. This seems to be a result of an issue called "TOCTOU."
Ceabed_API bool file_exists(const char *file_path){
    bool result = false;
    File file;
    if(file_open(&file, file_path, File_Flag_Read)){
        result = true;
        file_close(&file);
    }
    return result;
}

typedef struct {
    DIR           *info;
    struct dirent* contents;
} File__Dir;

typedef struct{
    File_Type   file_type;
    const char *file_name;

    u32        dirs_count;
    u32        dirs_used;
    File__Dir *dirs;
    u32        path_count;
    u32        path_used;
    char      *path;
} File__Walker;

static_assert(sizeof(File_Walker) >= sizeof(File__Walker));

#if 1
static void file__walker_append_path(File__Walker *s, const char *path){
    // TODO: Actually append the path here!
    size_t len = strlen(path);
    assert(len < s->path_count);
    memcpy(s->path, path, len);
}

static void file__walker_pop_path(File__Walker *s){

}

Ceabed_API void file_walker_begin(File_Walker *walker, const char *dir_path){
    File__Walker *s = (File__Walker *)walker;
    memset(s, 0, sizeof(File__Walker));

    s->dirs_count = 16;
    s->dirs = (File__Dir *)malloc(s->dirs_count * sizeof(File__Dir));
    s->path_count = 4096;
    s->path = (char*)malloc(s->path_count);

    s->file_type = File_Type_Directory;
    s->file_name = dir_path;
    file_walker_enter_directory(walker);
}

Ceabed_API void file_walker_end(File_Walker *walker){
    File__Walker *s = (File__Walker *)walker;
    if(s->dirs) free(s->dirs);
    if(s->path) free(s->path);
}

Ceabed_API bool file_walker_advance(File_Walker *walker){
    File__Walker *s = (File__Walker *)walker;

    bool result = false;
    while(s->dirs_used > 0){
        File__Dir *entry = &s->dirs[s->dirs_used-1];
        struct dirent *next = readdir(entry->info);
        if(next){
            const char *name = next->d_name;
            if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

            result = true;
            s->file_name = name;
            switch(next->d_type){
                default: s->file_type = File_Type_Unknown; break;

                case DT_DIR: s->file_type = File_Type_Directory; break;
                case DT_REG: s->file_type = File_Type_File; break;

                case DT_UNKNOWN:{
                    // TODO: We should try and determine the file type using other means. lstat?
                    assert(0);
                } break;
            }

            break;
        }
        else{
            closedir(entry->info);
            s->dirs_used--;
            file__walker_pop_path(s);
        }
    }

    return result;
}

Ceabed_API void file_walker_enter_directory(File_Walker *walker){
    File__Walker *s = (File__Walker *)walker;
    if(s->file_type == File_Type_Directory){
        file__walker_append_path(s, s->file_name);

        DIR *dir = opendir(s->path);
        if(dir){
            if(s->dirs_used == s->dirs_count){
                s->dirs_count *= 2;
                s->dirs = realloc(s->dirs, s->dirs_count);
            }

            File__Dir *entry = &s->dirs[s->dirs_used++];
            entry->info = dir;
        }
        else{
            fmt_msg("Unable to walk directory {0}: {1}\n", fmt_cstr(s->path), fmt_cstr(strerror(errno)));
        }
    }
    else{
        fmt_msg("Unable to walk {0}: Not a directory.\n", fmt_cstr(s->path));
    }
}

Ceabed_API String file_walker_make_path(File_Walker *walker, Buffer *buffer){
    String result = {};

    return result;
}

#endif

#endif // OS_Linux
//------------------------------------------------------------------------------
// OS_Linux
//------------------------------------------------------------------------------
