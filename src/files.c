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
#ifdef __gnu_linux__

#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

bool file_open(File *file, const char *file_path, uint32_t flags){
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

size_t file_read(File *file, size_t offset, void *buffer, size_t buffer_size){
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

void file_write(File *file, size_t offset, void *buffer, size_t buffer_size){
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

size_t file_stream_in(File *file, void *buffer, size_t buffer_size){
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

void file_stream_out(File *file, void *buffer, size_t buffer_size){
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

void file_close(File* file){
    assert(file->flags & File_Flag_Is_Open);
    int fd = (int)file->handle;
    close(fd);
    file->flags = 0;
}

size_t file_get_size(File* file){
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

File file_get_stdin(){
    uint32_t flags = File_Flag_Is_Open|File_Flag_Read;
    File result = {flags, 0};
    return result;
}

File file_get_stdout(){
    uint32_t flags = File_Flag_Is_Open|File_Flag_Write;
    File result = {flags, 1};
    return result;
}

File file_get_stderr(){
    uint32_t flags = File_Flag_Is_Open|File_Flag_Write;
    File result = {flags, 2};
    return result;
}

void delete_file(const char *file_path){
    // TODO: Error handling?
    unlink(file_path);
}

#endif // __gnu_linux__
//------------------------------------------------------------------------------
// OS_Linux
//------------------------------------------------------------------------------
