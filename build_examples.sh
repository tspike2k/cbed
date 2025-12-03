LIBS='-lX11 -lXi -lGL -lm'
FLAGS='-g -Wall -Isrc '

gcc $FLAGS -o ./bin/rect examples/rect.c src/display.c $LIBS
gcc $FLAGS -o ./bin/box examples/box.c src/display.c $LIBS
