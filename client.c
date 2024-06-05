#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ncurses.h>

#include "game_options.h"

void display_game_state(char map[MAP_HEIGHT][MAP_WIDTH]) {
    initscr();
    noecho();
    curs_set(FALSE);

    // Display the map
    for (int i = 0; i < MAP_HEIGHT; ++i) {
        for (int j = 0; j < MAP_WIDTH; ++j) {
            mvaddch(i, j, map[i][j]);
        }
    }

    // Refresh to show the changes
    refresh();
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

    while(1){
        read(sock, map, sizeof(map));
        display_game_state(map);
    }

    endwin();
    close(sock);
    return 0;
}
