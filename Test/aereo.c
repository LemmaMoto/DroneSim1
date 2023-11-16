#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

struct Object {
    int x;
    int y;
    char symbol;
};

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

    struct Object objects[10];
    for (int i = 0; i < 10; i++) {
        objects[i].x = rand() % (COLS - 1);
        objects[i].y = rand() % (LINES - 1);
        objects[i].symbol = 'O';
    }

    struct Drone drone = {0, 0, 'W', 1};

    int obstacle_x[10];
    int obstacle_y[10];
    char obstacle_symbol = 'X';

    int ch;
    bool obstacle_visible[10] = {false};
    time_t last_obstacle_time[10];

    int remaining_objects = 10;

    for (int i = 0; i < 10; i++) {
        last_obstacle_time[i] = time(NULL);
    }

    while (remaining_objects > 0) {
        clear();

        attron(COLOR_PAIR(drone.color_pair));
        mvaddch(drone.y, drone.x, drone.symbol);
        attroff(COLOR_PAIR(drone.color_pair));

        for (int i = 0; i < 10; i++) {
            mvaddch(objects[i].y, objects[i].x, objects[i].symbol);
        }

        for (int i = 0; i < 10; i++) {
            if (obstacle_visible[i]) {
                mvaddch(obstacle_y[i], obstacle_x[i], obstacle_symbol);
            }
        }

        refresh();

        int target_x = -1;
        int target_y = -1;
        double min_distance = 99999;

        ch = getch();

        if (ch == 'q') {
            break;
        }

        for (int i = 0; i < 10; i++) {
            if (objects[i].x == -1) {
                continue;
            }

            double distance = sqrt(pow(objects[i].x - drone.x, 2) + pow(objects[i].y - drone.y, 2));

            if (distance < min_distance) {
                min_distance = distance;
                target_x = objects[i].x;
                target_y = objects[i].y;
            }
        }

        // Logica di movimento del drone
        if (target_x != -1) {
            int dx = (target_x > drone.x) ? 1 : (target_x < drone.x) ? -1 : 0;
            int dy = (target_y > drone.y) ? 1 : (target_y < drone.y) ? -1 : 0;

            // Verifica se ci sono ostacoli sulla strada del drone
            bool obstacle_in_path = false;
            int obstacle_index = -1;
            
            for (int i = 0; i < 10; i++) {
                if (obstacle_visible[i] &&
                    ((obstacle_x[i] == drone.x + dx && obstacle_y[i] == drone.y + dy) ||
                     (obstacle_x[i] == drone.x + dx && obstacle_y[i] == drone.y) ||
                     (obstacle_x[i] == drone.x && obstacle_y[i] == drone.y + dy))) {
                    obstacle_in_path = true;
                    obstacle_index = i;
                    break;
                }
            }

            // Se c'è un ostacolo sulla strada, cambia direzione
            if (!obstacle_in_path) {
                drone.x += dx;
                drone.y += dy;
            } else {
                // Trovato un ostacolo sulla traiettoria, cerca di evitarlo
                int dx_obstacle = obstacle_x[obstacle_index] - drone.x;
                int dy_obstacle = obstacle_y[obstacle_index] - drone.y;
                if (abs(dx_obstacle) > abs(dy_obstacle)) {
                    drone.x -= dx_obstacle > 0 ? 1 : -1;
                } else {
                    drone.y -= dy_obstacle > 0 ? 1 : -1;
                }
            }
        }

        if (min_distance < 0.1) {
            for (int i = 0; i < 10; i++) {
                if (objects[i].x == target_x && objects[i].y == target_y) {
                    objects[i].x = -1;
                    remaining_objects--;
                }
            }
        }
        // Logica di generazione ostacoli
        // Genera un ostacolo ogni secondo, per due secondi
        for (int i = 0; i < 10; i++) {
            if (!obstacle_visible[i] && difftime(time(NULL), last_obstacle_time[i]) >= 1) {
                obstacle_visible[i] = true;
                last_obstacle_time[i] = time(NULL);
                obstacle_x[i] = rand() % (COLS - 1);
                obstacle_y[i] = rand() % (LINES - 1);
            } else if (obstacle_visible[i] && difftime(time(NULL), last_obstacle_time[i]) >= 2) {
                obstacle_visible[i] = false;
                last_obstacle_time[i] = time(NULL);
            }
        }

        usleep(1000);
    }

    endwin();
    return 0;
}
