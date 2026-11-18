#
# GNU makefile for the cbed project. This file is used to build the example programs.
#
FLAGS := -g -Wall -Wno-unused-function -Wno-unused-variable -Isrc -L./bin -fvisibility=hidden
LIBS := -lX11 -lXi -lGL -lm

all: display rect box tga cat walk_files fmt pad font_builder hotload_code

./bin/display.o:
	display

display:
	$(CC) -c $(FLAGS) -DDisplay_OpenGL ./src/display.c -o ./bin/display.o $(LIBS)

rect: display
	$(CC) $(FLAGS) -o ./bin/rect examples/rect.c ./bin/display.o $(LIBS)

box: display
	$(CC) $(FLAGS) -o ./bin/box examples/box.c ./bin/display.o $(LIBS)

tga:
	$(CC) $(FLAGS) -o ./bin/tga examples/tga.c

cat:
	$(CC) $(FLAGS) -o ./bin/cat examples/cat.c

walk_files:
	$(CC) $(FLAGS) -o ./bin/walk_files examples/walk_files.c

fmt:
	$(CC) $(FLAGS) -o ./bin/fmt examples/fmt.c

pad: display
	$(CC) $(FLAGS) -o ./bin/pad examples/pad.c ./bin/display.o $(LIBS)

font_builder:
	$(CC) $(FLAGS) -I/usr/include/freetype2 -o ./bin/font_builder examples/font_builder.c -lfreetype -lm

json:
	$(CC) $(FLAGS) -o ./bin/json examples/parse_json.c

mandelbrot: display
	$(CC) $(FLAGS) -o ./bin/mandelbrot examples/mandelbrot.c ./bin/display.o $(LIBS)

d_test:
	$(CC) -c $(FLAGS) -DDisplay_OpenGL ./src/display.c ./src/common.c -lX11 -lXi -lGL
	ldc -g -of=./bin/d_test examples/d_test.d ./display.o ./common.o -L=-lX11 -L=-lXi -L=-lGL

#NOTE: Experimental
ui: display
	$(CC) $(FLAGS) -o ./bin/ui examples/ui.c ./bin/display.o $(LIBS)

#NOTE: Experimental
vulkan_test:
	$(CC) $(FLAGS) -o ./bin/vulkan_test examples/vulkan_test.c -lX11 -lXi -lm -lvulkan

# This would use the Vulkan display backend.
#$(CC) -c $(FLAGS) -DDisplay_Vulkan ./src/display.c -o ./bin/display_vk.o $(LIBS)
#$(CC) $(FLAGS) -o ./bin/vulkan_test examples/vulkan_test.c ./bin/display_vk.o -lX11 -lXi -lm -lvulkan

# NOTE: The code hotloading example is compiled differently than the others because it has
# unusual requirements. In typical scenarios a library provides global symbols and the base
# executable links against them. In our case, however, we would like to do the opposite;
# the base executable contains all the platform-specific code and we would like to export that
# to the hotloaded shared library. GCC doesn't place global symbols of executable into the global
# symbol table by default, but we can force this by using the -rdynamic flag. Unfortunatly,
# it will export ALL dynamic symbols, but it does work.
#
# It might be a better idea to instead compile cbed as a seperate library and link to that,
# using the -rpath=$ORIGIN trick to prevent the user form needing to install the lib. It would
# be worth checking to see how well cbed works as a shared library anyway.
#
# To check which symbols are in the dynamic symbol table, use 'nm -D <binary_name>'.
hotload_code: display
	$(CC) -g -Wall -Isrc -shared -nodefaultlibs -nostartfiles -o ./bin/libhotload.so examples/hotload_lib.c
	$(CC) $(FLAGS) -rdynamic -o ./bin/hotload examples/hotload.c ./bin/display.o $(LIBS)
