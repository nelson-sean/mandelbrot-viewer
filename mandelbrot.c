#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <math.h>

///////////////////////////
// Structure definitions //
///////////////////////////
typedef struct {

    long double a;
    long double b;

}complex_t;

typedef struct {

	long double min_x;
	long double max_x;

	long double min_y;
	long double max_y;

	int screen_height;
	int screen_width;

}window_t;

//////////////////////////
// Function definitions //
//////////////////////////
void init_ncurses();

complex_t complex_multiply(complex_t x, complex_t y);
complex_t complex_add(complex_t x, complex_t y);
complex_t complex_sub(complex_t x, complex_t y);
long double complex_magnitude(complex_t x);

void draw_info_bar(window_t display);


int main(int argc, char **argv){

	window_t display;

	display.min_x = -2;
	display.max_x = 1;
	display.min_y = -1;
	display.max_y = 1;
	display.screen_height  = LINES - 1;
	display.screen_width = COLS;

    init_ncurses();

	draw_info_bar(display);

    getch();
    endwin();
    exit(0);
    
}

void init_ncurses(){

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    //nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();

	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_WHITE);

}

void draw_info_bar(window_t display){

	attron(COLOR_PAIR(2));

	mvprintw(0, 0, "%*s", COLS, " ");
	mvprintw(0, 0, "Real Axis: %.3Lf - %.3Lf | Imaginary Axis: %.3Lf - %.3Lf",
			display.min_x, display.max_x,
			display.min_y, display.max_y);

	attroff(COLOR_PAIR(2));
	

}

complex_t complex_multiply(complex_t x, complex_t y){

	complex_t result;

	result.a = (x.a * y.a) - (x.b * y.b);
	result.b = (x.a * y.b) + (y.a * x.b);

	return result;

}

complex_t complex_add(complex_t x, complex_t y){

	complex_t result;

	result.a = x.a + y.a;
	result.b = x.b + y.b;

	return result;

}

complex_t complex_sub(complex_t x, complex_t y){

	complex_t result;

	result.a = x.a - y.a;
	result.b = x.b - y.b;

	return result;

}

// TODO: Either implement sqrt or ask about using math.h
long double complex_magnitude(complex_t x){

	long double result = sqrt(x.a * x.a + x.b * x.b);

	return result;

}
