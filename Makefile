all:
	gcc -Wall -g mandelbrot.c -o mandelbrot -lform -lmenu -lncurses -lm
