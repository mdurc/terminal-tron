#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#include "game_options.h"

typedef struct {
    int sockfd,id;
    char shape;
    int x,y,size;
    vec path[MAX_LEN];
    vec dir;
} client_t;

client_t CLIENTS[PLAYER_COUNT];
const char user_chars[2] = {'@', '#'};

int PLAYING = 1;

void gen_map(char map[MAP_HEIGHT][MAP_WIDTH]) {
    // Initialize map with empty spaces
    int i,j;
    for(i=0;i<MAP_HEIGHT;++i){
        for(j=0;j<MAP_WIDTH;++j){
            map[i][j] = '.';
        }
    }    
    for(i=0;i<PLAYER_COUNT;++i){
        map[CLIENTS[i].y][CLIENTS[i].x] = CLIENTS[i].shape;
    }
}


int find_collisions(client_t* usr){
    // check for collisions with self.
    // usr->size-1 is the head (exclude it in checking)
    int i, z=usr->size-2;
    vec* head = &usr->path[z+1];
    for(i=0;i<z;++i){
        if(head->x == usr->path[i].x && head->y == usr->path[i].y){
            return 1;
        }
    }
    for(i=0;i<PLAYER_COUNT;++i){
        if(usr->id == CLIENTS[i].id){
            continue;
        }
        // go through all the coordinates of the other path
        int j,z=CLIENTS[i].size-1;
        for(j=0;j<z;++j){
            if(head->x == CLIENTS[i].path[j].x && head->y == CLIENTS[i].path[j].y){
                return 1;
            }
        }
    }
    return 0;
}

void update_game_state(char map[MAP_HEIGHT][MAP_WIDTH], client_t* usr){
    // wrap around if they go out of bounds
    usr->y = (usr->y + usr->dir.y + MAP_HEIGHT) % MAP_HEIGHT;
    usr->x = (usr->x + usr->dir.x + MAP_WIDTH) % MAP_WIDTH;

    map[usr->y][usr->x] = usr->shape;

    if(find_collisions(usr)){
        printf("Client %d died\n", usr->id);
        PLAYING = 0;
        return;
    }

    // keep track of all the spots the bike has traveled.
    usr->path[usr->size] = (vec){usr->x, usr->y};
    if(++usr->size >= MAX_LEN){
        printf("Client %d wins\n", usr->id);
        PLAYING = 0;
    }
}


void start_processes() {
    char map[MAP_HEIGHT][MAP_WIDTH];
    gen_map(map);

    while(PLAYING){
        // Send the same map to all clients
        int i;
        vec newInput;
        for(i=0;i<PLAYER_COUNT;++i){
            // send out the current map
            write(CLIENTS[i].sockfd, map, sizeof(map));
            // read the next movement
            read(CLIENTS[i].sockfd, &newInput, sizeof(vec));
            if(!(newInput.x == 0 && newInput.y == 0)){ CLIENTS[i].dir = newInput; }
            //printf("Game state sent to client #%d.\n", i);
        }
        for(i=0;i<PLAYER_COUNT;++i){
            // update based on the input
            update_game_state(map,&CLIENTS[i]);
        }
        usleep(FRAME_TIME);
    }

}

void create_player(client_t* usr, int* new_socket, int num_players){
    usr->sockfd = *new_socket;
    usr->id = num_players;
    usr->shape = user_chars[num_players];
    // Generate random starting coords
    usr->x = rand() % MAP_WIDTH;
    usr->y = rand() % MAP_HEIGHT;
    usr->path[0] = (vec){usr->x,usr->y};
    usr->size=1;

    // make a random starting direction
    do{ usr->dir = (vec){rand() % 3 - 1, rand() % 3 - 1}; }
    while(!((usr->dir.x==0 && usr->dir.y!=0) || (usr->dir.x!=0 && usr->dir.y==0)));
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char map[MAP_HEIGHT][MAP_WIDTH];
    int start_x, start_y;
    srand(time(NULL));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Waiting for a connection...\n");

    // accepts two clients
    int num_players=0;
    for(;num_players<2;++num_players) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            return 1;
        }

        printf("Client #%d joined\n", num_players);
        create_player(&CLIENTS[num_players], &new_socket, num_players);
    }
    printf("Starting Game\n");
    start_processes();

    close(new_socket);
    close(server_fd);
    return 0;
}
