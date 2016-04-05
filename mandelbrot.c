#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <math.h>

#define BARSIZE 20

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

complex_t scale(window_t display, int row, int column);

void draw_info_bar(window_t display);

int is_in_set(complex_t c);

int main(int argc, char **argv){

    init_ncurses();

	window_t display;
    WINDOW *fractal_window;

	display.min_x = -2;
	display.max_x = 1;
	display.min_y = -1;
	display.max_y = 1;
	display.screen_height  = LINES - 2;
	display.screen_width = COLS-BARSIZE-2;


	draw_info_bar(display);

    fractal_window = newwin(LINES, COLS-BARSIZE, 0, BARSIZE);
    wborder(fractal_window, '|', '|', '-', '-', '+', '+', '+', '+');

    int row, col;
    for(row = 0; row < display.screen_height; row++){
        for(col = 0; col < display.screen_width; col++){
            complex_t c = scale(display, row, col);
            if(!is_in_set(c)){
                mvwprintw(fractal_window, row+1, col+1, "X");
            }
        }
    }


    refresh();
    wrefresh(fractal_window);

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

    mvprintw(0, 0, "Real Axis:");
    mvprintw(1, 0, "  min: %.5Lf", display.min_x);
    mvprintw(2, 0, "  max: %.5Lf", display.max_x);

    mvprintw(4, 0, "Imaginary Axis:");
    mvprintw(5, 0, "  min: %.5Lf", display.min_y);
    mvprintw(6, 0, "  min: %.5Lf", display.max_y);


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

complex_t scale(window_t display, int row, int col){

    // window coords include border so subtract from row and column values to get actual coord on plane
    int actual_x = col - 1;
    int actual_y = row - 1;

    complex_t c;

    long double x_cursor_units = (display.max_x - display.min_x)/display.screen_width;
    long double y_cursor_units = (display.max_y - display.min_y)/display.screen_height;

    c.a = display.min_x + (actual_x * x_cursor_units);
    c.b = display.max_y - (actual_y * y_cursor_units);

    return c;

}

int is_in_set(complex_t c){

    // initial z set to 0
    complex_t z;    
    z.a = 0;
    z.b = 0;

    int i = 0;
    do{

        complex_t result = complex_add(complex_multiply(z, z), c);

        if(complex_magnitude(result) >= 2){
            return false;
        }

        z = result;
        i++;

    }while(i < 10);

    return true;


}
