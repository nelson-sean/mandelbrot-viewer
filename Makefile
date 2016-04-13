all:
	gcc -Wall -g mandelbrot.c -o mandelbrot -lform -lncurses -lm
