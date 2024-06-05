#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ncurses.h>

#include "game_options.h"

void display_game_state(char map[MAP_HEIGHT][MAP_WIDTH]) {
    int i,j;
    for(i=0;i<MAP_HEIGHT;++i){
        for(j=0;j<MAP_WIDTH;++j){
            mvaddch(i, j, map[i][j]);
        }
    }
    refresh();
}


vec readInput(WINDOW** win) {
    int pressed = wgetch(*win);
    vec dir;
    if (pressed == KEY_LEFT) { dir.x = -1; dir.y = 0; }
    else if (pressed == KEY_RIGHT) { dir.x = 1; dir.y = 0; }
    else if (pressed == KEY_UP) { dir.x = 0; dir.y = -1; }
    else if (pressed == KEY_DOWN) { dir.x = 0; dir.y = 1; }
    else { dir.x = 0; dir.y = 0; }
    return dir;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char map[MAP_HEIGHT][MAP_WIDTH];
    int start_x, start_y;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    WINDOW* win = initscr();
    keypad(win, true);
    noecho();
    nodelay(win, true);
    curs_set(0);

    while(1){
        vec in = readInput(&win);
        read(sock, map, sizeof(map));
        display_game_state(map);
        write(sock, &in, sizeof(vec));
    }

    endwin();
    close(sock);
    return 0;
}
