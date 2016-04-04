#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>

void init_ncurses();

typedef struct complex_t {
    long double a;
    long double i;
}

int main(int argc, char **argv){

    init_ncurses();

    mvprintw(10, 10, "wassup");

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

}
