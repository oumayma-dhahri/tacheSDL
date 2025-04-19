all:
	gcc -Wall -Wextra -g -DSDL_COMPAT main.c source.c -o prog -lSDL -lSDL_image -lSDL_ttf -lm

run: prog
	./prog

clean:
	rm -f prog

