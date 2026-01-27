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


#if 0
typedef struct {
    const char* name;
    DIR         dir;
    dirent* entry_stream;
} File__Dir_Node;

typedef struct {
    File__Dir_Node *nodes;
    uint32_t        nodes_used;
    uint32_t        nodes_length;
} File__Walker;

static void file__walk_dir(File__Walker *walker, const char *dir_name){

}

void push_directory(String dir_name){
        auto dir = opendir(dir_name.ptr);
        if(dir){
            Node* node;
            if(node_first_free){
                node = node_first_free;
                node_first_free = node.next;
            }
            else{
                node = alloc_type!Node(scratch);
            }
            node.path = dir_name;
            nodes.insert(nodes.top, node);
            node.dir = dir;
        }
    }


File_Walker file_recurse_dir(const char *dir, const char *file_name_buffer, size_t file_name_buffer_size){
    File_Walker result = {};
    assert(sizeof(File__Walker) <= sizeof(File_Walker));
    File__Walker *walker = (File__Walker*)&result;




    return result;
}

file_recurse_next(File_Walker* walker){

}
#endif

#endif // OS_Linux
//------------------------------------------------------------------------------
// OS_Linux
//------------------------------------------------------------------------------
