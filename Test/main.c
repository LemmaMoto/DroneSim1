#include <ncurses.h>

int main()
{
	initscr();
        raw();
	int derp =4;
	printw("This is bog standart string output %d", derp);
	addch('a');
	move(12,13);

	mvprintw(15,20,"movement");
	mvaddch(12,50,'@');
	getch();
	endwin();
	
	

	return 0;
}
