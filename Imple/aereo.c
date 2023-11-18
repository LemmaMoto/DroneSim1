#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

struct Drone {
    int x;
    int y;
    char symbol;
    short color_pair;
};

int main() {
    initscr();
    start_color();
    curs_set(0);
    timeout(100);
    srand(time(NULL));

    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    struct Drone drone = {0, 0, 'W', 1};
    mvprintw(drone.y, drone.x, "%c", drone.symbol); // Print the drone symbol at the drone position

    refresh(); // Refresh the screen to show the changes

    sleep(3); // Wait for 5 seconds so you can see the output

    endwin();
    return 0;
}