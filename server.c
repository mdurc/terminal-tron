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

// Globals:
char map[MAP_HEIGHT][MAP_WIDTH];
client_t CLIENTS[PLAYER_COUNT];
const char user_chars[2] = {'@', '#'};
int PLAYING = 1;

// Functions:

void fill_map() {
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


void respawn_player(client_t* usr){
    // clear previous path on the map
    int i,z=usr->size-1;
    for(i=0;i<z;++i){
        map[usr->path[i].y][usr->path[i].x] = '.';
    }

    // Generate random starting coords
    usr->x = rand() % MAP_WIDTH;
    usr->y = rand() % MAP_HEIGHT;

    map[usr->y][usr->x] = usr->shape; // place the new spawn location

    usr->path[0] = (vec){usr->x,usr->y};
    usr->size=1;
    // make a random starting direction
    do{ usr->dir = (vec){rand() % 3 - 1, rand() % 3 - 1}; }
    while(!((usr->dir.x==0 && usr->dir.y!=0) || (usr->dir.x!=0 && usr->dir.y==0)));
}


int find_collisions(client_t* usr){
    vec* head = &usr->path[usr->size-1];
    if(map[head->y][head->x] == usr->shape){
        printf("Client %d collided with self\n", usr->id);
        return 1;
    }
    else if(map[head->y][head->x] != '.'){
        printf("Client %d collided with opponent\n", usr->id);
        return 1;
    }
    return 0;
}

void update_game_state(client_t* usr){
    // wrap around if they go out of bounds
    usr->y = (usr->y + usr->dir.y + MAP_HEIGHT) % MAP_HEIGHT;
    usr->x = (usr->x + usr->dir.x + MAP_WIDTH) % MAP_WIDTH;

    if(find_collisions(usr)){
        // this is the only loss condition
        printf("Client %d died\n", usr->id);
        respawn_player(usr);
        return;
    }

    map[usr->y][usr->x] = usr->shape;
    // keep track of all the spots the bike has traveled.
    usr->path[usr->size] = (vec){usr->x, usr->y};
    if(++usr->size >= MAX_LEN){
        printf("Client %d wins\n", usr->id);
        PLAYING = 0;
    }
}

int validNewDirection(vec* curr, vec* new){
    if(new->x == 0 && new->y == 0){
        // when the input is not one of the arrow keys, or is invalid
        return 0;
    }else if((abs(curr->x)+abs(new->x)==2)||(abs(curr->y)+abs(new->y)==2)){
        // trying to move in the opposite or same direction
        return 0;
    }
    return 1;
}


void start_processes() {
    fill_map();

    while(PLAYING){
        // Send the same map to all clients
        int i;
        vec newInput;
        for(i=0;i<PLAYER_COUNT;++i){
            // send out the current map
            write(CLIENTS[i].sockfd, map, sizeof(map));
            // read the next movement
            read(CLIENTS[i].sockfd, &newInput, sizeof(vec));
            if(validNewDirection(&CLIENTS[i].dir, &newInput)){
                CLIENTS[i].dir = newInput;
            }

            //printf("{%d, %d} in direction: {%d, %d}\n", CLIENTS[i].x, CLIENTS[i].y, CLIENTS[i].dir.x, CLIENTS[i].dir.y);
            //printf("Game state sent to client #%d.\n", i);
        }
        for(i=0;i<PLAYER_COUNT;++i){
            // update based on the input
            update_game_state(&CLIENTS[i]);
        }
        usleep(FRAME_TIME);
    }

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
        CLIENTS[num_players].sockfd = new_socket;
        CLIENTS[num_players].id = num_players;
        CLIENTS[num_players].shape = user_chars[num_players];
        respawn_player(&CLIENTS[num_players]);
    }
    printf("Starting Game\n");
    start_processes();

    close(new_socket);
    close(server_fd);
    return 0;
}
