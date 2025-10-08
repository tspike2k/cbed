/*
Authors:   tspike (github.com/tspike2k)
Copyright: Copyright (c) 2024
License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
*/

#include "../src/files.c"
#include <stdlib.h>

#define Buffer_Size 32
char buffer[Buffer_Size];

int main(int args_count, const char** args){
    File stdout = file_get_stdout();

    if(args_count > 1){
        // Arguments passed to the application are expected to be file names. Therefore we try
        // to open each argument as a file and print its contents to stdout.
        File f;
        for(int i = 0; i < args_count-1; i++){
            if(file_open(&f, args[i+1], File_Flag_Read)){
                size_t size = file_get_size(&f);
                void *memory = malloc(size+1);
                file_read(&f, 0, memory, size);
                ((char*)memory)[size] = '\n';

                file_stream_out(&stdout, memory, size+1);
                free(memory);

                file_close(&f);
            }
        }
    }
    else{
        // No arguments were passed to the application. In that case we should instead print text
        // piped to this application by reading it from stdin.
        File stdin = file_get_stdin();

        size_t bytes_read;
        while(bytes_read = file_stream_in(&stdin, &buffer[0], Buffer_Size), bytes_read > 0){ // Silly use of the comma operator
            file_stream_out(&stdout, buffer, bytes_read);
        }
    }

    return 0;
}
