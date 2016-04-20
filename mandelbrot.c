#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <ncurses.h>
#include <form.h>
#include <menu.h>
#include <math.h>
#include <string.h>

#define BARSIZE 18
#define MAX_ITERATIONS 100

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

typedef enum {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    ZOOM_OUT,
    ZOOM_IN
}WINDOW_ACTION;

typedef enum {
    GOLDEN_PURPLE = 0,
    PASTEL_RAINBOW = 1,
    SCARLET_GRAY = 2,
    OCEAN = 3,
    EARTH = 4
}COLOR_PALETTE;

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
double is_in_set(complex_t c);
void open_menu(window_t *display);
void open_bitmap_menu(window_t *display);
COLOR_PALETTE open_palette_menu(window_t *display);
void draw_bitmap(char *file_name, window_t display, int image_width, int image_height, COLOR_PALETTE colors);
unsigned char **get_gradient_palette(unsigned char color1[3], unsigned char color2[3], int samples);
unsigned char **create_palette(COLOR_PALETTE colors);
void free_palette(unsigned char **palette, COLOR_PALETTE colors);
void trim_string(char *string);


///////////////////////////////////////
// main:                             //
//   initialization and key handling //
///////////////////////////////////////
int main(int argc, char **argv){

    // initialize ncurses options
    init_ncurses();

    // create window_t object and ncurses window
    window_t display;
    WINDOW *fractal_window;

    // set display default values
    display.min_x = -2;
    display.max_x = 1;
    display.min_y = -1;
    display.max_y = 1;
    display.screen_height  = LINES - 2;
    display.screen_width = COLS-BARSIZE-2;


    // draw info bar to left of fractal window
    draw_info_bar(display);

    // create new window for displaying fractal
    fractal_window = newwin(LINES, COLS-BARSIZE, 0, BARSIZE);

    // draw fractal to new window
    draw_fractal_window(fractal_window, display);

    // update screen
    refresh();
    wrefresh(fractal_window);

    int quit = FALSE;
    while(!quit){

        // handle keyboard input
        switch(getch()){

            // move fractal rendering left
            case 'a':
                move_window(fractal_window, &display, LEFT);
            break;

            // move fractal rendering right
            case 'd':
                move_window(fractal_window, &display, RIGHT);
            break;

            // move fractal rendering up
            case 'w':
                move_window(fractal_window, &display, UP);
            break;

            // move fractal rendering down
            case 's':
                move_window(fractal_window, &display, DOWN);
            break;

            // zoom fractal rendering in
            case 'e':
                move_window(fractal_window, &display, ZOOM_IN);
            break;

            // zoom fractal rendering out
            case 'q':
                move_window(fractal_window, &display, ZOOM_OUT);
            break;

            // open axis menu
            case 'm':

                open_menu(&display);
                clear();

                // redraw display after menu closes
                draw_info_bar(display);
                draw_fractal_window(fractal_window, display);

            break;

            // capture ascii codes for '`' and '~'
            // open bitmap menus and draw to file
            case 96:
            case 126:

                open_bitmap_menu(&display);
                clear();

                // redraw display after menu closes
                draw_info_bar(display);
                draw_fractal_window(fractal_window, display);

            break;

            // handle terminal resize event
            case KEY_RESIZE:

                display.screen_height  = LINES - 2;
                display.screen_width = COLS-BARSIZE-2;

                wresize(fractal_window, LINES, COLS-BARSIZE);

                draw_info_bar(display);
                draw_fractal_window(fractal_window, display);

            break;

            // capture ascii code for escape key
            case 27:
                quit = TRUE;
            break;

        }

    }

    // cleanly destroy window and exit program
    endwin();
    exit(1);
    
}



/////////////////////////////////////////
// init_ncurses:                       //
//   setup ncurses and ncurses options //
/////////////////////////////////////////
void init_ncurses(){

    initscr();
    cbreak(); // don't wait for return to process input
    noecho(); // don't write input to screen
    keypad(stdscr, true); // enable keypad on main window
    curs_set(0); // hide cursor
    start_color(); // enable color
    use_default_colors(); // assign terminal defaults to -1

    // initialize colors, -1 is default terminal background
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_YELLOW, -1);
    init_pair(3, COLOR_GREEN, -1);
    init_pair(4, COLOR_CYAN, -1);
    init_pair(5, COLOR_BLUE, -1);
    init_pair(6, COLOR_MAGENTA, -1);


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
    mvprintw(6, 0, "  max: %.5Lf", display.max_y);


}



//////////////////////////////////////////////////////////////////////////////////////
// draw_fractal_window:                                                             //
//   draw border around fractal window and draw appropriate cursor positions with X //
//////////////////////////////////////////////////////////////////////////////////////
void draw_fractal_window(WINDOW *fractal_window, window_t display){

    // clear display for redrawing
    wclear(fractal_window);

    // redraw border around window
    //wborder(fractal_window, '|', '|', '-', '-', '+', '+', '+', '+');
    box(fractal_window, 0, 0);

    // calculate color and print
    int row, col;
    for(row = 0; row < display.screen_height; row++){
        for(col = 0; col < display.screen_width; col++){

            // use screen coordinate to find corresponding number on complex plane
            complex_t c = scale(display, row, col);

            // get normalized escape value
            double mu = is_in_set(c);

            // if not 0, point is not in set, find color
            if(mu != 0){

                // get normalized escape to be between 1 and 6 for ncurses colors
                int color_num = (int)floor(mu) % 6 + 1;

                // turn on color, write X, and turn off color
                wattron(fractal_window, COLOR_PAIR(color_num));
                mvwprintw(fractal_window, row+1, col+1, "X");
                wattroff(fractal_window, COLOR_PAIR(color_num));

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

    // calculate number of units on complex planes corresponding to the width and height of one character
    long double x_cursor_units = (display->max_x - display->min_x)/display->screen_width;
    long double y_cursor_units = (display->max_y - display->min_y)/display->screen_height;

    // used in aspect ratio calculations when zooming
    long double x_length, y_length, x_diff, aspect;

    switch(action){

        // move fractal display left one cursor unit
        case LEFT:
            display->max_x = display->max_x - x_cursor_units;
            display->min_x = display->min_x - x_cursor_units;
        break;

        // move fractal display right one cursor unit
        case RIGHT:
            display->max_x = display->max_x + x_cursor_units;
            display->min_x = display->min_x + x_cursor_units;
        break;

        // move fractal display up one cursor unit
        case UP:
            display->max_y = display->max_y + y_cursor_units;
            display->min_y = display->min_y + y_cursor_units;
        break;

        // move fractal display down one cursor unit
        case DOWN:
            display->max_y = display->max_y - y_cursor_units;
            display->min_y = display->min_y - y_cursor_units;
        break;

        // zoom fractal display in
        case ZOOM_IN:

            aspect = (display->max_x - display->min_x) / (display->max_y - display->min_y);

            display->min_y += y_cursor_units;
            display->max_y -= y_cursor_units;

            // calculate real axis values to maintain current aspect ratio
            y_length = display->max_y - display->min_y;
            x_length = y_length * aspect;

            x_diff = (display->max_x - display->min_x) - x_length;
            display->min_x += (x_diff/2);
            display->max_x -= (x_diff/2);


        break;

        // zoom fractal display out
        case ZOOM_OUT:

            aspect = (display->max_x - display->min_x) / (display->max_y - display->min_y);

            display->min_y -= y_cursor_units;
            display->max_y += y_cursor_units;

            // calculate new real axis values to maintain current aspect ratio (3:2)
            y_length = display->max_y - display->min_y;
            x_length = y_length * aspect;

            x_diff = x_length - (display->max_x - display->min_x);
            display->min_x -= (x_diff/2);
            display->max_x += (x_diff/2);

        break;

    }

    // redraw info bar and fractal window
    draw_info_bar(*display);
    draw_fractal_window(fractal_window, *display);

}



//////////////////////////////////////////////////////////////////////////////////
// open_menu:                                                                   //
//   open an ncurses menu with fields to change the current window_t parameters //
//////////////////////////////////////////////////////////////////////////////////
void open_menu(window_t *display){

    // create ncurses window and form pointers
    WINDOW* menu_win = newwin(10, 50, 5, 5);
    FIELD *fields[5];
    FORM *form;
    int ch, rows, cols;

    // define necessary fields and terminating NULL field
    fields[0] = new_field(1, 15, 1, 6, 0, 0);
    fields[1] = new_field(1, 15, 2, 6, 0, 0);
    fields[2] = new_field(1, 15, 5, 6, 0, 0);
    fields[3] = new_field(1, 15, 6, 6, 0, 0);
    fields[4] = NULL;

    // set field options
    int i;
    for(i = 0; i < 4; i++){

        set_field_back(fields[i], A_UNDERLINE); // underline field
        field_opts_off(fields[i], O_AUTOSKIP);  // don't move to next field when full

    }

    // create string buffers for grabbing current display parameters
    char *real_min_string = malloc(15*sizeof(char));
    char *real_max_string = malloc(15*sizeof(char));
    char *imag_min_string = malloc(15*sizeof(char));
    char *imag_max_string = malloc(15*sizeof(char));

    // read current display paramters
    sprintf(real_min_string, "%.5Lf", display->min_x);
    sprintf(real_max_string, "%.5Lf", display->max_x);
    sprintf(imag_min_string, "%.5LF", display->min_y);
    sprintf(imag_max_string, "%.5LF", display->max_y);

    // write current display parameters to corresponding fields
    set_field_buffer(fields[0], 0, real_min_string);
    set_field_buffer(fields[1], 0, real_max_string);
    set_field_buffer(fields[2], 0, imag_min_string);
    set_field_buffer(fields[3], 0, imag_max_string);

    // create form using defined fields
    form = new_form(fields);

    // put the necessary size of the form subwindow into ints, rows and cols
    scale_form(form, &rows, &cols);

    // create form main window using rows and cols as reference and place in center of screen
    menu_win = newwin(rows+4, cols+11, (LINES/2)-((rows+4)/2), (COLS/2)-((cols+11)/2));

    // enable keypad on new window
    keypad(menu_win, TRUE);

    // associate window with form
    set_form_win(form, menu_win);
    // create required subwindow for form
    set_form_sub(form, derwin(menu_win, rows, cols, 1, 1));

    // post form to window
    post_form(form);
    
    // draw border around menu window
    box(menu_win, 0, 0);

    // refresh windows and draw to display
    wrefresh(menu_win);
    refresh();

    // Write field labels
    mvwprintw(menu_win, 1, 1, "Real Axis:");
    mvwprintw(menu_win, 2, 2, "min: ");
    mvwprintw(menu_win, 3, 2, "max: ");

    mvwprintw(menu_win, 5, 1, "Imaginary Axis:");
    mvwprintw(menu_win, 6, 2, "min: ");
    mvwprintw(menu_win, 7, 2, "max: ");

    // write instructions to bottom of menu window
    mvwprintw(menu_win, 9, 2, "m to confirm | ESC to cancel");
    
    // refresh menu window
    wrefresh(menu_win);

    // make cursor visible
    curs_set(1);

    // move cursor to end of first field
    form_driver(form, REQ_FIRST_FIELD);
    form_driver(form, REQ_END_LINE);

    int done = FALSE;
    // loop until fields are confirmed or escape is pressed
    while(!done && (ch = wgetch(menu_win)) != 27){
        switch(ch){

            // move to field above current
            case KEY_UP:
                form_driver(form, REQ_PREV_FIELD);
                form_driver(form, REQ_END_LINE);
            break;

            // move to field below current
            case '\t':
            case '\n':
            case KEY_DOWN:
                form_driver(form, REQ_NEXT_FIELD);
                form_driver(form, REQ_END_LINE);
            break;

            // move cursor one character to the left
            case KEY_LEFT:
                form_driver(form, REQ_PREV_CHAR);
            break;

            // move cursor one character to the right
            case KEY_RIGHT:
                form_driver(form, REQ_NEXT_CHAR);
            break;

            // delete one character
            case KEY_BACKSPACE:
            case 127:
                form_driver(form, REQ_PREV_CHAR);
                form_driver(form, REQ_DEL_CHAR);
            break;

            // validate fields and update display values
            case 'm':
                form_driver(form, REQ_VALIDATION);

                display->min_x = atof(field_buffer(fields[0], 0));
                display->max_x = atof(field_buffer(fields[1], 0));
                display->min_y = atof(field_buffer(fields[2], 0));
                display->max_y = atof(field_buffer(fields[3], 0));

                done = TRUE;

            break;

            case KEY_RESIZE:

                display->screen_height  = LINES - 2;
                display->screen_width = COLS-BARSIZE-2;

            break;

            // if character is a valid part of a double print to current field
            default:
                if(isdigit(ch) || ch == '-' || ch == '.'){
                    form_driver(form, ch);
                }
            break;
        }
    }

    // hide cursor
    curs_set(0);

    // clean up ncurses objects
    unpost_form(form);
    free_form(form);
    free_field(fields[0]);
    free_field(fields[1]);
    free_field(fields[2]);
    free_field(fields[3]);
    free(real_min_string);
    free(real_max_string);
    free(imag_min_string);
    free(imag_max_string);
    delwin(menu_win);
    
}

///////////////////////////////////////////////////////////////////////////////////
// open_bitmap_menu:                                                             //
//   open options menu and form for exporting currently viewed fractal to bitmap //
///////////////////////////////////////////////////////////////////////////////////
void open_bitmap_menu(window_t *display){

    // define all necessary form/menu items
    WINDOW *menu_win = newwin(10, 50, COLS/2, LINES/2);
    FIELD *fields[4];
    FORM *resolution_form;
    COLOR_PALETTE palette;
    int ch, rows, cols;

    // open menu for user to choose desired color palette
    palette = open_palette_menu(display);

    // define form fields including necessary NULL
    fields[0] = new_field(1, 15, 2, 11, 0, 0);
    fields[1] = new_field(1, 15, 3, 11, 0, 0);
    fields[2] = new_field(1, 15, 5, 11, 0, 0);
    fields[3] = NULL;

    // set field options
    int i;
    for(i = 0; i < 3; i++){

        set_field_back(fields[i], A_UNDERLINE);
        field_opts_off(fields[i], O_AUTOSKIP);

    }

    // set default values in fields to be 100
    set_field_buffer(fields[0], 0, "100");
    set_field_buffer(fields[1], 0, "100");
    set_field_buffer(fields[2], 0, "fractal.bmp");

    // create form using defined fields
    resolution_form = new_form(fields);

    // place necessary size of form subwindow into rows and cols
    scale_form(resolution_form, &rows, &cols);

    // calculate menu_window size using rows and cols and place in middle of screen
    menu_win = newwin(rows+4, cols+8, (LINES/2)-((rows+4)/2), (COLS/2)-((cols+8))/2);

    // enable keypad on menu window
    keypad(menu_win, TRUE);

    // associate menu_window with form
    set_form_win(resolution_form, menu_win);

    // associate subwindow with form
    set_form_sub(resolution_form, derwin(menu_win, rows, cols, 1, 1));

    // draw border around menu window
    box(menu_win, 0, 0);

    // post form to window
    post_form(resolution_form);

    // write instructions and field labels to menu window
    mvwprintw(menu_win, 1, ((cols+8)/2)-7, "Bitmap Export");
    mvwprintw(menu_win, 3, 5, "Width: ");
    mvwprintw(menu_win, 4, 4, "Height: ");
    mvwprintw(menu_win, 6, 2, "Filename: ");
    mvwprintw(menu_win, rows+2, ((cols+8)/2)-14, "~ to confirm | ESC to cancel");

    // refresh and redraw windows
    wrefresh(menu_win);
    refresh();

    // move cursor to end of first field
    form_driver(resolution_form, REQ_FIRST_FIELD);
    form_driver(resolution_form, REQ_END_LINE);

    // show cursor
    curs_set(1);

    int done = FALSE;
    // loop until user cancels or confirms resolution choice
    while(!done && (ch = wgetch(menu_win)) != 27){

        // handle keyboard input
        switch(ch){

            int image_width, image_height;
            char *file_name;

            // ascii codes for '`' and '~' respectively
            // validate values in current fields and draw bitmap of that size using chosen palette
            case 96:
            case 126:

                curs_set(0);
                form_driver(resolution_form, REQ_VALIDATION);

                image_width = atoi(field_buffer(fields[0], 0));
                image_height = atoi(field_buffer(fields[1], 0));

                file_name = field_buffer(fields[2], 0);
                trim_string(file_name);

                draw_bitmap(file_name, *display, image_width, image_height, palette);

                done = TRUE;
                
            break;

            // tab, return, or down arrow will move to next field
            case '\t':
            case '\n':
            case KEY_DOWN:

                form_driver(resolution_form, REQ_NEXT_FIELD);
                form_driver(resolution_form, REQ_END_LINE);

            break;

            // move to previous field
            case KEY_UP:

                form_driver(resolution_form, REQ_PREV_FIELD);
                form_driver(resolution_form, REQ_END_LINE);

            break;

            // move cursor to previous character
            case KEY_LEFT:

                form_driver(resolution_form, REQ_PREV_CHAR);

            break;

            // move cursor to next character
            case KEY_RIGHT:

                form_driver(resolution_form, REQ_NEXT_CHAR);

            break;

            // delete previous character
            case KEY_BACKSPACE:
            case 127:

                form_driver(resolution_form, REQ_PREV_CHAR);
                form_driver(resolution_form, REQ_DEL_CHAR);

            break;

            // delete current character
            case KEY_DC:

                form_driver(resolution_form, REQ_DEL_CHAR);

            break;

            case KEY_RESIZE:

                display->screen_height = LINES - 2;
                display->screen_width = COLS - BARSIZE - 2;

            break;

            // if input is digit, write to current field
            default:

                form_driver(resolution_form, ch);

            break;
        }
    }

    // hide cursor
    curs_set(0);

    // clean up ncurses objects
    unpost_form(resolution_form);
    free_form(resolution_form);
    free_field(fields[0]);
    free_field(fields[1]);
    delwin(menu_win);


}

//////////////////////////////////////////////////////////////////////////
// open_palette_menu:                                                   //
//   open menu for user to pick desired color palette for bitmap export //
//   returns COLOR_PALETTE enum corresponding to user choice            //
//////////////////////////////////////////////////////////////////////////
COLOR_PALETTE open_palette_menu(window_t *display){

    // define ncurses objects
    WINDOW *palette_window;
    ITEM **palette_items;
    ITEM *choice;
    MENU *palette_menu;
    COLOR_PALETTE palette;
    int ch, done;
    int n_choices = 5;

    // default value in case of error somewhere
    palette = GOLDEN_PURPLE;

    // Color choices for palette menu
    char *choices[] = {
        "Golden Purple",
        "Pastel Rainbow",
        "Scarlet and Gray",
        "Ocean",
        "Earth"
    };

    // allocate memory for menu items
    palette_items = NULL;
    palette_items = malloc((n_choices+1) * sizeof(ITEM *));

    // create new menu items using each string in choices
    int i;
    for(i = 0; i < n_choices; i++){
        palette_items[i] = new_item(choices[i], choices[i]);
    }

    // define a null item to signify end of item list
    palette_items[i] = NULL;

    // create menu
    palette_menu = new_menu(palette_items);

    // create window for menu
    palette_window = newwin(10, 26, (LINES/2)-5, (COLS/2)-13);
    keypad(palette_window, TRUE);

    // associate menu's main and subwindow
    set_menu_win(palette_menu, palette_window);
    set_menu_sub(palette_menu, derwin(palette_window, n_choices, 18, 4, 3));

    set_menu_mark(palette_menu, ">");

    // print border around window
    box(palette_window, 0, 0);

    // refresh main screen
    refresh();

    // print menu message
    mvwprintw(palette_window, 1, 2, "Choose a color palette");

    // post menu and draw menu window
    post_menu(palette_menu);
    wrefresh(palette_window);

    // grab input until enter is pressed
    done = FALSE;
    while(!done){
        ch = wgetch(palette_window);
        switch(ch){

            // move selection down
            case KEY_DOWN:
                menu_driver(palette_menu, REQ_DOWN_ITEM);
            break;

            // move selection up
            case KEY_UP:
                menu_driver(palette_menu, REQ_UP_ITEM);
            break;

            // select currently highlighted item
            case '\n':

                // set choice to item currently highlighted in menu
                choice = current_item(palette_menu);
                done = TRUE;

            break;

            // handle terminal resize event
            case KEY_RESIZE:

                display->screen_height = LINES - 2;
                display->screen_width = COLS - BARSIZE - 2;

            break;

        }
        wrefresh(palette_window);
    }


    // compare string in menu choice to strings originally defined
    for(i = 0; i < n_choices; i++){
        if(strcmp(choice->name.str, choices[i]) == 0){

            // if matched set return value to i
            palette = i;

        }
    }

    // clean up ncurses objects
    unpost_menu(palette_menu);
    for(i = 0; i < n_choices; i++){
        free_item(palette_items[i]);
    }
    free(palette_items);
    free_item(choice);
    free_menu(palette_menu);
    delwin(palette_window);

    palette_menu = NULL;
    palette_items = NULL;
    palette_window = NULL;
    choice = NULL;

    refresh();

    return palette;
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
    int actual_x = col;
    int actual_y = row;

    complex_t c;

    // calculate number of complex units corresponding to one cursor width or height
    long double x_cursor_units = (display.max_x - display.min_x)/display.screen_width;
    long double y_cursor_units = (display.max_y - display.min_y)/display.screen_height;

    // calculate position on complex plane relative to position on display
    c.a = display.min_x + (actual_x * x_cursor_units);
    c.b = display.max_y - (actual_y * y_cursor_units);

    return c;

}



///////////////////////////////////////////////////////////////////////////////
// is_in_set:                                                                //
//   return 0 if complex_t c is in the mandelbrot set and mu, a normalized   //
//   valued related to the escape time of the recursive function             //
//   this is used in calculating the color of each coordinate                //
///////////////////////////////////////////////////////////////////////////////
double is_in_set(complex_t c){

    // initial z set to 0
    complex_t z;    
    z.a = 0;
    z.b = 0;

    // set escape radius to 2
    double escape_r = 2.0;
    double mu;

    // i is iterations completed
    int i = 0;
    while(i <= MAX_ITERATIONS){

        // iterate z value and store result
        z = complex_add(complex_multiply(z, z), c);

        // increment iteration counter
        i++;
        double mag = complex_magnitude(z);

        // if mag is greater than escape radius point is not in set, end loop
        if (mag > escape_r){
            break;
        }
    }

    // if c is in set calculate mu, normalized escape value
    if(i < MAX_ITERATIONS){

        // complete a couple more iterations of z to get cleaner mu value
        z = complex_add(complex_multiply(z, z), c);
        i++;
        z = complex_add(complex_multiply(z, z), c);
        i++;
           
        double mag = complex_magnitude(z);
        mu = i - ( log( log(mag) ) / log(2.0) );

        // handle occasional NaN results from calculation
        if(isnan(mu)){
            mu = 0;
        }

        // handle negative mu values
        if(mu < 0){
            mu *= -1;
        }

    }else{

        // else set mu to exactly zero
        mu = 0;

    }

    return mu;

}



void draw_bitmap(char *file_name, window_t display, int image_width, int image_height, COLOR_PALETTE colors){

    window_t bitmap_window;

    bitmap_window.min_x = display.min_x;
    bitmap_window.max_x = display.max_x;
    bitmap_window.min_y = display.min_y;
    bitmap_window.max_y = display.max_y;

    bitmap_window.screen_height = image_height;
    bitmap_window.screen_width = image_width;


    FILE *image;
    image = fopen(file_name, "wb");

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

    unsigned char **palette = create_palette(colors);
    
    // smooth coloring experiment
    int row, col;
    for(row = bitmap_window.screen_height - 1; row >= 0; row--){
        for(col = 0; col < bitmap_window.screen_width; col++){

            complex_t c = scale(bitmap_window, row, col);

            double mu = is_in_set(c);

            if(mu != 0){

                int color1, color2;
                switch(colors){

                    // 3 color palettes
                    case GOLDEN_PURPLE:
                    case SCARLET_GRAY:

                        color1 = (int)floor(mu) % 8;
                        color2 = ((int)floor(mu) + 1) % 8;

                    break;

                    case OCEAN:

                        color1 = (int)floor(mu) % 9;
                        color2 = ((int)floor(mu) + 1) % 9;

                    break;

                    // 4 color palettes
                    case PASTEL_RAINBOW:
                    case EARTH:

                        color1 = (int)floor(mu) % 12;
                        color2 = ((int)floor(mu)+1) % 12;

                    break;

                }

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


    free_palette(palette, colors);
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

unsigned char **create_palette(COLOR_PALETTE colors){

    // GOLDEN_PURPLE colors
    unsigned char gp_purple[] = {0x72, 0x02, 0x61};
    unsigned char gp_gold[] = {0x1b, 0x80, 0x99};
    unsigned char gp_blue[] = {0x88, 0x5e, 0x06};

    // PASTEL_RAINBOW colors
    unsigned char pr_blue[] = {0x6a, 0x4e, 0x23};
    unsigned char pr_green[] = {0x31, 0x80, 0x26};
    unsigned char pr_yellow[] = {0x30, 0x72, 0xa5};
    unsigned char pr_red[] = {0x30, 0x36, 0xa5};

    // SCARLET_GRAY colors
    unsigned char sg_red[] = {0x0, 0x0, 0xbb};
    unsigned char sg_gray[] = {0x66, 0x66, 0x66};
    unsigned char sg_white[] = {0xff, 0xff, 0xff};

    // OCEAN colors
    unsigned char o_lightgreen[] = {0x29, 0xd4, 0x9f};
    unsigned char o_bluegreen[] = {0x5d, 0x94, 0x14};
    unsigned char o_turquoise[] = {0x4d, 0x67, 0x0b};
    unsigned char o_marine[] = {0x43, 0x45, 0x0a};

    // EARTH colors
    unsigned char e_darkgreen[] = {0x00, 0x4d, 0x33};
    unsigned char e_lightgreen[] = {0x18, 0x9e, 0x90};
    unsigned char e_beige[] = {0x74, 0x85, 0xa1};
    unsigned char e_brown[] = {0x2a, 0x43, 0x77};

    // final palette to be returned
    unsigned char **palette;

    // gradient palettes to be combined
    unsigned char **palette1;
    unsigned char **palette2;
    unsigned char **palette3;

    // for array indexing later
    int i;
    

    switch(colors){

        case GOLDEN_PURPLE:

            palette = malloc(8 * sizeof(unsigned char*));
            
            palette1 = get_gradient_palette(gp_gold, gp_purple, 4);
            palette2 = get_gradient_palette(gp_purple, gp_blue, 4);
            for(i=0; i < 4; i++){

                palette[i] = palette1[i];
                palette[i+4] = palette2[i];

            }

        break;

        case PASTEL_RAINBOW:

            palette = malloc(12 * sizeof(unsigned char*));

            palette1 = get_gradient_palette(pr_blue, pr_red, 4);
            palette2 = get_gradient_palette(pr_red, pr_yellow, 4);
            palette3 = get_gradient_palette(pr_yellow, pr_green, 4);

            for(i = 0; i < 4; i++){

                palette[i] = palette1[i];
                palette[i+4] = palette2[i];
                palette[i+8] = palette3[i];

            }

        break;

        case SCARLET_GRAY:

            palette = malloc(8 * sizeof(unsigned char*));

            palette1 = get_gradient_palette(sg_gray, sg_red, 4);
            palette2 = get_gradient_palette(sg_red, sg_white, 4);
            for(i=0; i < 4; i++){

                palette[i] = palette1[i];
                palette[i+4] = palette2[i];

            }

        break;

        case OCEAN:

            palette = malloc(9 * sizeof(unsigned char*));

            palette1 = get_gradient_palette(o_lightgreen, o_bluegreen, 3);
            palette2 = get_gradient_palette(o_bluegreen, o_turquoise, 3);
            palette3 = get_gradient_palette(o_turquoise, o_marine, 3);
            for(i = 0; i < 3; i++){

                palette[i] = palette1[i];
                palette[i+3] = palette2[i];
                palette[i+6] = palette3[i];

            }

        break;

        case EARTH:

            palette = malloc(12 * sizeof(unsigned char*));

            palette1 = get_gradient_palette(e_beige, e_brown, 4);
            palette2 = get_gradient_palette(e_brown, e_lightgreen, 4);
            palette3 = get_gradient_palette(e_lightgreen, e_darkgreen, 4);
            for(i = 0; i < 4; i++){

                palette[i] = palette1[i];
                palette[i+4] = palette2[i];
                palette[i+8] = palette3[i];

            }

        break;

    }

    palette1 = NULL;
    palette2 = NULL;
    palette3 = NULL;
    return palette;

}

void free_palette(unsigned char **palette, COLOR_PALETTE colors){

    int i;

    switch(colors){

        // 8 color palettes
        case GOLDEN_PURPLE:
        case SCARLET_GRAY:

            for(i = 0; i < 8; i++){
                free(palette[i]);
            }

        break;

        // 9 color palettes
        case OCEAN:

            for(i = 0; i < 9; i++){
                free(palette[i]);
            }


        break;


        // 12 color palettes
        case PASTEL_RAINBOW:
        case EARTH:

            for(i = 0; i < 12; i++){
                free(palette[i]);
            }

        break;

    }

    free(palette);

}

void trim_string(char *string){

    int i;
    for(i = 0; i < strlen(string); i++){
        if(isspace(string[i])){
            string[i] = '\0';
            break;
        }
    }

}
