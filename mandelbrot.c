#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <math.h>
#include <string.h>

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

    int iterations;

}window_t;

typedef enum {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    ZOOM_OUT,
    ZOOM_IN
}WINDOW_ACTION;

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
void draw_fractal_window(WINDOW *fractal_window, window_t display);
void move_window(WINDOW *fractal_window, window_t *display, WINDOW_ACTION action);
int is_in_set(complex_t c);
void open_menu(window_t *display);


///////////////////////////////////////
// main:                             //
//   initialization and key handling //
///////////////////////////////////////
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
    display.iterations = 1000;


	draw_info_bar(display);

    fractal_window = newwin(LINES, COLS-BARSIZE, 0, BARSIZE);

    draw_fractal_window(fractal_window, display);

    refresh();
    wrefresh(fractal_window);

    while(1){

        switch(getch()){

            case 'a':
                move_window(fractal_window, &display, LEFT);
            break;

            case 'd':
                move_window(fractal_window, &display, RIGHT);
            break;

            case 'w':
                move_window(fractal_window, &display, UP);
            break;

            case 's':
                move_window(fractal_window, &display, DOWN);
            break;

            case 'e':
                move_window(fractal_window, &display, ZOOM_IN);
            break;

            case 'q':
                move_window(fractal_window, &display, ZOOM_OUT);
            break;

            case 'm':
                open_menu(&display);
                clear();
                draw_info_bar(display);
                draw_fractal_window(fractal_window, display);
            break;

            // Escape key
			case 27:
                endwin();
                exit(1);
			break;

        }

    }

    endwin();
    exit(0);
    
}



/////////////////////////////////////////
// init_ncurses:                       //
//   setup ncurses and ncurses options //
/////////////////////////////////////////
void init_ncurses(){

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    curs_set(0);
    //nodelay(stdscr, TRUE);
    start_color();
    use_default_colors();

    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);


}



//////////////////////////////////////////////////////////////
// draw_info_bar:                                           //
//   display information to the left of the fractal window  //
//////////////////////////////////////////////////////////////
void draw_info_bar(window_t display){

    mvprintw(0, 0, "Real Axis:");
    mvprintw(1, 0, "  min: %.5Lf", display.min_x);
    mvprintw(2, 0, "  max: %.5Lf", display.max_x);

    mvprintw(4, 0, "Imaginary Axis:");
    mvprintw(5, 0, "  min: %.5Lf", display.min_y);
    mvprintw(6, 0, "  min: %.5Lf", display.max_y);


}



//////////////////////////////////////////////////////////////////////////////////////
// draw_fractal_window:                                                             //
//   draw border around fractal window and draw appropriate cursor positions with X //
//////////////////////////////////////////////////////////////////////////////////////
void draw_fractal_window(WINDOW *fractal_window, window_t display){

    // clear display for redrawing
    wclear(fractal_window);

    // redraw border around window
    wborder(fractal_window, '|', '|', '-', '-', '+', '+', '+', '+');

    // create histogram and escape size buffer for coloring (doubles for hue calculations)
    double histogram[display.iterations];
    int calculation_buffer[display.screen_height][display.screen_width];

    memset(histogram, 0, sizeof(histogram));
    memset(calculation_buffer, 0, sizeof(calculation_buffer));

    // loop row by row and column by column populating histogram and escape size buffer
    int row, col;
    for(row = 0; row < display.screen_height; row++){
        for(col = 0; col < display.screen_width; col++){

            // find complex number corresponding to on screen coord and determine if it's in the set
            complex_t c = scale(display, row, col);
            int escape = is_in_set(c);

            // if escape > 0 coord is not in set
            if(escape > 0){
                calculation_buffer[row][col] = escape;
                histogram[escape-1] += 1;
            }else{
                escape = 0;
            }

        }
    }

    // total number of colored blocks in display
    double total = 0.0;
    int i;
    for(i = 0; i < display.iterations; i++){
        total += (double)histogram[i];
    }

    // calculate color and print
    for(row = 0; row < display.screen_height; row++){
        for(col = 0; col < display.screen_width; col++){

            // this will produce a hue number between 0 and 1
            long double hue_num = 0.0;
            if(calculation_buffer[row][col] > 0){

                int escape = calculation_buffer[row][col];

                int i;
                for(i = 0; i < escape-1; i++){
                    long double quotient = histogram[i]/total;
                    hue_num += quotient;
                }

                // hue is between 0 and 1 but we need between 1 and 6
                hue_num *= 5;
                hue_num += 1;

                int color = round(hue_num);

                wattron(fractal_window, COLOR_PAIR(color));
                mvwprintw(fractal_window, row+1, col+1, "X");
                wattroff(fractal_window, COLOR_PAIR(color));

            }


        }
    }

    refresh();
    wrefresh(fractal_window);

}



////////////////////////////////////////
// move_window:                       //
//   handle window actions and redraw //
////////////////////////////////////////
void move_window(WINDOW *fractal_window, window_t *display, WINDOW_ACTION action){

    long double x_cursor_units = (display->max_x - display->min_x)/display->screen_width;
    long double y_cursor_units = (display->max_y - display->min_y)/display->screen_height;

    switch(action){

        case LEFT:
            display->max_x = display->max_x - x_cursor_units;
            display->min_x = display->min_x - x_cursor_units;
        break;

        case RIGHT:
            display->max_x = display->max_x + x_cursor_units;
            display->min_x = display->min_x + x_cursor_units;
        break;

        case UP:
            display->max_y = display->max_y + y_cursor_units;
            display->min_y = display->min_y + y_cursor_units;
        break;

        case DOWN:
            display->max_y = display->max_y - y_cursor_units;
            display->min_y = display->min_y - y_cursor_units;
        break;

        case ZOOM_IN:

            display->min_x += x_cursor_units;
            display->max_x -= x_cursor_units;

            display->min_y += y_cursor_units;
            display->max_y -= y_cursor_units;

        break;

        case ZOOM_OUT:
            display->min_x -= x_cursor_units;
            display->max_x += x_cursor_units;

            display->min_y -= y_cursor_units;
            display->max_y += y_cursor_units;
        break;

    }

    draw_info_bar(*display);
    draw_fractal_window(fractal_window, *display);

}



void open_menu(window_t *display){

    WINDOW* menu_win = newwin(10, 50, 5, 5);
    box(menu_win, 0, 0);
    wrefresh(menu_win);

    getch();

    delwin(menu_win);
    
}


/////////////////////////////////////////////////////////////////////////
// complex_multiply:                                                   //
//   multiply two complex_t numbers and return the resulting complex_t //
/////////////////////////////////////////////////////////////////////////
complex_t complex_multiply(complex_t x, complex_t y){

	complex_t result;

	result.a = (x.a * y.a) - (x.b * y.b);
	result.b = (x.a * y.b) + (y.a * x.b);

	return result;

}



/////////////////////////////////////////////////////////////////////////
// complex_add:                                                        //
//   add two complex_t numbers and return the resulting complex_t      //
/////////////////////////////////////////////////////////////////////////
complex_t complex_add(complex_t x, complex_t y){

	complex_t result;

	result.a = x.a + y.a;
	result.b = x.b + y.b;

	return result;

}



///////////////////////////////////////////////////////////////////////////////////
// complex_sub:                                                                  //
//   subtract one complex_t from another and return the resulting complex_t      //
///////////////////////////////////////////////////////////////////////////////////
complex_t complex_sub(complex_t x, complex_t y){

	complex_t result;

	result.a = x.a - y.a;
	result.b = x.b - y.b;

	return result;

}



// TODO: Either implement sqrt or ask about using math.h
///////////////////////////////////////////
// complex_magnitude:                    //
//   return the magnitude of a complex_t //
///////////////////////////////////////////
long double complex_magnitude(complex_t x){

	long double result = sqrt(x.a * x.a + x.b * x.b);

	return result;

}



//////////////////////////////////////////////////////////////////////////
// complex_scale:                                                       //
//   return the complex_t corresponding to the given cursor coordinates //
//////////////////////////////////////////////////////////////////////////
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



///////////////////////////////////////////////////////////////////////////////
// is_in_set:                                                                //
//   return true if complex_t c is in the mandelbrot set and false otherwise //
///////////////////////////////////////////////////////////////////////////////
// TODO: this is not to the lab specification, consider creating an exit_value function or sumpin
int is_in_set(complex_t c){

    // initial z set to 0
    complex_t z;    
    z.a = 0;
    z.b = 0;

    int i = 1;
    do{

        complex_t result = complex_add(complex_multiply(z, z), c);

        if(complex_magnitude(result) >= 2){
            return i;
        }

        z = result;
        i++;

    }while(i <= 1000);

    return 0;


}
