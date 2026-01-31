LIBS='-lX11 -lXi -lGL -lm'
FLAGS='-g -Wall -Isrc -fvisibility=hidden -Wno-unused-function -Wno-unused-variable'

gcc $FLAGS -c ./src/display.c -o ./bin/display.o $LIBS
gcc $FLAGS -o ./bin/rect examples/rect.c ./bin/display.o $LIBS
gcc $FLAGS -o ./bin/box examples/box.c ./bin/display.o $LIBS
gcc $FLAGS -o ./bin/tga examples/tga.c -lm
gcc $FLAGS -o ./bin/cat examples/cat.c -lm
