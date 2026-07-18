#!/bin/bash
FLAGS='-g -Wall -Wno-unused-function -Wno-unused-variable -Isrc -L./bin -fvisibility=hidden '
LIBS='-lX11 -lXi -lGL -lm '

gcc -c $FLAGS ./src/display.c -o ./bin/display.o $LIBS
#ar rcs ./bin/libdisplay.a ./bin/display.o

#gcc $FLAGS -c ./src/display.c -o ./bin/display.o $LIBS
#gcc $FLAGS -o ./bin/rect examples/rect.c ./bin/display.o $LIBS
#gcc $FLAGS -o ./bin/box examples/box.c ./bin/display.o $LIBS
#gcc $FLAGS -o ./bin/tga examples/tga.c
#gcc $FLAGS -o ./bin/cat examples/cat.c
#gcc $FLAGS -o ./bin/walk_files examples/walk_files.c
#gcc $FLAGS -o ./bin/fmt examples/fmt.c
#gcc $FLAGS -o ./bin/pad examples/pad.c ./bin/display.o $LIBS
gcc $FLAGS -I/usr/include/freetype2 -o ./bin/font_builder examples/font_builder.c -lfreetype -lm



gcc -g -Wall -Isrc -shared -nodefaultlibs -nostartfiles -o ./bin/libhotload.so examples/hotload_lib.c

# NOTE: The code hotloading example is compiled differently than the others because it has
# unusual requirements. In typical scenarios a library provides global symbols and the base
# executable links against them. In our case, however, we would like to do the opposite;
# the base executable contains all the platform-specific code and we would like to export that
# to the hotloaded shared library. GCC doesn't place global symbols of executable into the global
# symbol table by default, but we can force this by using the -rdynamic flag. Unfortunatly,
# it will export ALL dynamic symbols, but it does work.
#
# It might be a better idea to instead compile ceabed as a seperate library and link to that,
# using the -rpath=$ORIGIN trick to prevent the user form needing to install the lib. It would
# be worth checking to see how well ceabed works as a shared library anyway.
#
# To check which symbols are in the dynamic symbol talble, use 'nm -D <binary_name>'.
#gcc $FLAGS -DCeabed_API='__attribute__((visibility("default")))' -shared -o ./bin/libdisplay.so ./src/display.c $LIBS
#gcc $FLAGS -rdynamic -o ./bin/hotload examples/hotload.c  $LIBS -L./bin -ldisplay -Wl,-rpath,'$ORIGIN'
gcc $FLAGS -rdynamic -o ./bin/hotload examples/hotload.c ./bin/display.o $LIBS
