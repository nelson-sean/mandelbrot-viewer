#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <ncurses.h>
#include <form.h>
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
void open_bitmap_menu(window_t *display);
void draw_bitmap(window_t display, int image_width, int image_height);
unsigned char **get_gradient_palette(unsigned char color1[3], unsigned char color2[3], int samples);


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

	int quit = 0;

    while(!quit){

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

			// TODO: figure out how to grab the ~ key
			case 'b':
                open_bitmap_menu(&display);
                clear();
                draw_info_bar(display);
                draw_fractal_window(fractal_window, display);
			break;

            // Escape key
			case 27:
				quit = 1;
			break;

        }

    }

    endwin();
    exit(1);
    
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
	FIELD *fields[5];
	FORM *form;
	int ch, rows, cols;

	fields[0] = new_field(1, 15, 1, 6, 0, 0);
	fields[1] = new_field(1, 15, 2, 6, 0, 0);
	fields[2] = new_field(1, 15, 5, 6, 0, 0);
	fields[3] = new_field(1, 15, 6, 6, 0, 0);
	fields[4] = NULL;

	set_field_back(fields[0], A_UNDERLINE);
    field_opts_off(fields[0], O_AUTOSKIP);

	set_field_back(fields[1], A_UNDERLINE);
    field_opts_off(fields[1], O_AUTOSKIP);

	set_field_back(fields[2], A_UNDERLINE);
    field_opts_off(fields[2], O_AUTOSKIP);

	set_field_back(fields[3], A_UNDERLINE);
    field_opts_off(fields[3], O_AUTOSKIP);

	char *real_min_string = malloc(15*sizeof(char));
	char *real_max_string = malloc(15*sizeof(char));
	char *imag_min_string = malloc(15*sizeof(char));
	char *imag_max_string = malloc(15*sizeof(char));

	sprintf(real_min_string, "%.5Lf", display->min_x);
	sprintf(real_max_string, "%.5Lf", display->max_x);
	sprintf(imag_min_string, "%.5LF", display->min_y);
	sprintf(imag_max_string, "%.5LF", display->max_y);

	set_field_buffer(fields[0], 0, real_min_string);
	set_field_buffer(fields[1], 0, real_max_string);
	set_field_buffer(fields[2], 0, imag_min_string);
	set_field_buffer(fields[3], 0, imag_max_string);

	form = new_form(fields);

	scale_form(form, &rows, &cols);
	menu_win = newwin(rows+4, cols+11, (LINES/2)-((rows+4)/2), (COLS/2)-((cols+11)/2));
	keypad(menu_win, TRUE);

	set_form_win(form, menu_win);
	set_form_sub(form, derwin(menu_win, rows, cols, 1, 1));

	post_form(form);
	
    box(menu_win, 0, 0);
    wrefresh(menu_win);
	refresh();

	mvwprintw(menu_win, 1, 1, "Real Axis:");
	mvwprintw(menu_win, 2, 2, "min: ");
	mvwprintw(menu_win, 3, 2, "min: ");

	mvwprintw(menu_win, 5, 1, "Imaginary Axis:");
	mvwprintw(menu_win, 6, 2, "min: ");
	mvwprintw(menu_win, 7, 2, "min: ");

	mvwprintw(menu_win, 9, 2, "m to confirm | ESC to cancel");
	
	wrefresh(menu_win);

	curs_set(1);
    form_driver(form, REQ_FIRST_FIELD);
	form_driver(form, REQ_END_LINE);

	int done = 0;
	while(!done && (ch = wgetch(menu_win)) != 27){
		switch(ch){
			case KEY_UP:
				form_driver(form, REQ_PREV_FIELD);
				form_driver(form, REQ_END_LINE);
			break;

			case '\t':
			case '\n':
			case KEY_DOWN:
				form_driver(form, REQ_NEXT_FIELD);
				form_driver(form, REQ_END_LINE);
			break;

			case KEY_LEFT:
				form_driver(form, REQ_PREV_CHAR);
			break;

			case KEY_RIGHT:
				form_driver(form, REQ_NEXT_CHAR);
			break;

			case KEY_BACKSPACE:
			case 127:
				form_driver(form, REQ_PREV_CHAR);
				form_driver(form, REQ_DEL_CHAR);
			break;

			case 'm':
				form_driver(form, REQ_VALIDATION);

				display->min_x = atof(field_buffer(fields[0], 0));
				display->max_x = atof(field_buffer(fields[1], 0));
				display->min_y = atof(field_buffer(fields[2], 0));
				display->max_y = atof(field_buffer(fields[3], 0));

				done = 1;

			break;

			default:
				if(isdigit(ch) || ch == '-' || ch == '.'){
					form_driver(form, ch);
				}
			break;
		}
	}

	curs_set(0);

	unpost_form(form);
	free_form(form);
	free_field(fields[0]);
	free_field(fields[1]);
	free_field(fields[2]);
	free_field(fields[3]);
    delwin(menu_win);
    
}

void open_bitmap_menu(window_t *display){

    WINDOW *menu_win = newwin(10, 50, COLS/2, LINES/2);
    FIELD *fields[3];
    FORM *resolution_form;
    int ch, rows, cols;

    fields[0] = new_field(1, 15, 2, 9, 0, 0);
    fields[1] = new_field(1, 15, 3, 9, 0, 0);
    fields[2] = NULL;

    set_field_back(fields[0], A_UNDERLINE);
    field_opts_off(fields[0], O_AUTOSKIP);

    set_field_back(fields[1], A_UNDERLINE);
    field_opts_off(fields[1], O_AUTOSKIP);

	set_field_buffer(fields[0], 0, "100");
	set_field_buffer(fields[1], 0, "100");

    resolution_form = new_form(fields);

	// Note: this will set rows and cols to the size of the menu subwindow, menu_window will be of size
	// row+4 x col+4
    scale_form(resolution_form, &rows, &cols);
    menu_win = newwin(rows+4, cols+8, (LINES/2)-((rows+4)/2), (COLS/2)-((cols+4))/2);
    keypad(menu_win, TRUE);

    set_form_win(resolution_form, menu_win);
    set_form_sub(resolution_form, derwin(menu_win, rows, cols, 1, 1));

    box(menu_win, 0, 0);

    post_form(resolution_form);

    mvwprintw(menu_win, 1, ((cols+8)/2)-7, "Bitmap Export");
    mvwprintw(menu_win, 3, 3, "Width: ");
    mvwprintw(menu_win, 4, 2, "Height: ");
	mvwprintw(menu_win, rows+2, ((cols+8)/2)-14, "B to confirm | ESC to cancel");

    wrefresh(menu_win);
    refresh();

    form_driver(resolution_form, REQ_FIRST_FIELD);
    curs_set(1);

	int done = 0;
    while(!done && (ch = wgetch(menu_win)) != 27){
        switch(ch){

			int image_width, image_height;

			case 'b':
			case 'B':

				curs_set(0);
				form_driver(resolution_form, REQ_VALIDATION);

				image_width = atoi(field_buffer(fields[0], 0));
				image_height = atoi(field_buffer(fields[1], 0));
				draw_bitmap(*display, image_width, image_height);

				done = 1;
				
			break;

            case '\t':
            case '\n':
            case KEY_DOWN:

                form_driver(resolution_form, REQ_NEXT_FIELD);
                form_driver(resolution_form, REQ_END_LINE);

            break;

            case KEY_UP:

                form_driver(resolution_form, REQ_PREV_FIELD);
                form_driver(resolution_form, REQ_END_LINE);

            break;

			case KEY_LEFT:

				form_driver(resolution_form, REQ_PREV_CHAR);

			break;

			case KEY_RIGHT:

				form_driver(resolution_form, REQ_NEXT_CHAR);

			break;

            case KEY_BACKSPACE:
			case 127:

                form_driver(resolution_form, REQ_PREV_CHAR);
                form_driver(resolution_form, REQ_DEL_CHAR);

            break;

			case KEY_DC:

				form_driver(resolution_form, REQ_DEL_CHAR);

			break;

            default:

                if(isdigit(ch)){
                    form_driver(resolution_form, ch);
                }

            break;
        }
    }

    curs_set(0);

    unpost_form(resolution_form);
    free_form(resolution_form);
    free_field(fields[0]);
    free_field(fields[1]);
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



void draw_bitmap(window_t display, int image_width, int image_height){

	window_t bitmap_window;

	bitmap_window.min_x = display.min_x;
	bitmap_window.max_x = display.max_x;
	bitmap_window.min_y = display.min_y;
	bitmap_window.max_y = display.max_y;

	bitmap_window.screen_height = image_height;
	bitmap_window.screen_width = image_width;

	bitmap_window.iterations = 1000;

	FILE *image;
	image = fopen("fractal.bmp", "w");

	if(image == NULL){
		printf("error opening file for writing\n");
		exit(1);
	}

    int bytes_per_row = (((24 * bitmap_window.screen_width) + 31) / 32) * 4;
	int padding_bytes = bytes_per_row - (bitmap_window.screen_width * 3);

    // write BMP header

    char id[2] = {'B', 'M'};
    int size = bytes_per_row * bitmap_window.screen_height;
    short reserved = 0;
    int offset = 26;

    fwrite(id, 1, 2, image);
    fwrite(&size, 4, 1, image);
    fwrite(&reserved, 2, 2, image);
    fwrite(&offset, 4, 1, image);

    // write BITMAPCOREHEADER
    int header_size = 12;
    short width = bitmap_window.screen_width;
    short height = bitmap_window.screen_height;
    short color_planes = 1;
    short bpp = 24;

    fwrite(&header_size, 4, 1, image);
    fwrite(&width, 2, 1, image);
    fwrite(&height, 2, 1, image);
    fwrite(&color_planes, 2, 1, image);
    fwrite(&bpp, 2, 1, image);

	// TODO: fix color mess
	unsigned char **palette1;
	unsigned char **palette2;
	unsigned char **palette;

	palette = malloc(10 * sizeof(unsigned char*));
	

	unsigned char purple[] = {0x72, 0x02, 0x61};
	unsigned char gold[] = {0x1b, 0x80, 0x99};
	unsigned char blue[] = {0x88, 0x5e, 0x06};
	palette1 = get_gradient_palette(gold, purple, 5);
	palette2 = get_gradient_palette(purple, blue, 5);
	int asdf;
	for(asdf=0; asdf < 5; asdf++){
		palette[asdf] = palette1[asdf];
	}
	for(asdf=0; asdf < 5; asdf++){
		palette[asdf+5] = palette2[asdf];
	}

	// TODO: Add more color palettes and a choice
    
    // Green spectrum
//	palette[0] = (unsigned char[]){0xc6, 0xc8, 0xc5}; // whiteish
//	palette[1] = (unsigned char[]){0x95, 0xe0, 0xb3};
//	palette[2] = (unsigned char[]){0x5d, 0xbb, 0x82};
//	palette[3] = (unsigned char[]){0x32, 0x95, 0x59};
//  palette[4] = (unsigned char[]){0x13, 0x70, 0x37};
//  palette[5] = (unsigned char[]){0x00, 0x4b, 0x1d};

	// smooth coloring experiment
	int row, col;
	for(row = bitmap_window.screen_height - 1; row >= 0; row--){
		for(col = 0; col < bitmap_window.screen_width; col++){

			complex_t c = scale(bitmap_window, row, col);
			complex_t z;
			z.a = 0;
			z.b = 0;

			double escape_r = 2.0;
			int max_i = 100;

			// TODO: figure out how to get is_in_set back in here
			int i = 0;
			while(1){
				z = complex_add(complex_multiply(z, z), c);
				i++;
				double modulus = complex_magnitude(z);
				if (modulus > escape_r){
					break;
				}
				if(i > max_i){
					break;
				}
			}

            if(i < max_i){
                z = complex_add(complex_multiply(z, z), c);
                i++;
                z = complex_add(complex_multiply(z, z), c);
                i++;
                
                double modulus = complex_magnitude(z);
                double mu = i - ( log( log(modulus) ) / log(2.0) );

                if(isnan(mu)){
                    mu = 0;
                }

                if(mu < 0){
                    mu *= -1;
                }

                int color1 = (int)floor(mu) % 10;
                int color2 = ((int)floor(mu)+1) % 10;

                double blue = palette[color1][0] + ((palette[color2][0]-palette[color1][0]) * (mu-floor(mu)));
                double green = palette[color1][1] + ((palette[color2][1]-palette[color1][1]) * (mu-floor(mu)));
                double red = palette[color1][2] + ((palette[color2][2]-palette[color1][2]) * (mu-floor(mu)));
                unsigned char b = round(blue);
                unsigned char g = round(green);
                unsigned char r = round(red);
                char color[] = {b, g, r};

                fwrite(&color, 1, 3, image);
            }else{
                char black[] = {0, 0, 0};
                fwrite(&black, 1, 3, image);
            }
		}

		unsigned char zero = 0x0;
		fwrite(&zero, 1, padding_bytes, image);

	}
	
	fclose(image);
}

unsigned char **get_gradient_palette(unsigned char color1[3], unsigned char color2[3], int samples){

    unsigned char **palette;

    palette = malloc(samples * sizeof(unsigned char*));

    int i;
    for(i = 0; i < samples; i++){

        palette[i] = malloc(3 * sizeof(unsigned char));

        double progress = (1.0/samples) * i;

        double blue = color1[0] + ((color2[0] - color1[0]) * progress);
        double green = color1[1] + ((color2[1] - color1[1]) * progress);
        double red = color1[2] + ((color2[2] - color1[2]) * progress);
        unsigned char b = round(blue);
        unsigned char g = round(green);
        unsigned char r = round(red);

        palette[i][0] = b;
        palette[i][1] = g;
        palette[i][2] = r;
    }

    return palette;

}
