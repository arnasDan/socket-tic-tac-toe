#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "types.h"

void handle_error(const char* message)
{
    printf("An error occured: %s", message);
    pthread_exit(NULL);
}

void write_status(int client_socket, int status_code)
{
    int result = write(client_socket, &status_code, sizeof(int));
    if (result < 0)
        handle_error("Cannot write status to client socket");
}

void write_to_clients(int* clients, int status_code)
{
    write_status(clients[0], status_code);
    write_status(clients[1], status_code);
}
 
int setup_server(int port)
{ 
    int socket_no;
    struct sockaddr_in server_address;
    
    //             domain=IPv4, type=TCP,    protocol=default
    socket_no = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_no < 0)
        handle_error("Cannot open socket for listening");

    //setup server info
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    //Bind info to previously opened socket
    if (bind(socket_no, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
        handle_error("Cannot bind listener socket");
    
    printf("Now listening on %d", port);
    
    return socket_no;
}

void* run_game(void* thread_context)
{
    int* clients = (int*) thread_context;

    int game_over = 1;
    
    write_to_clients(clients, START);

    while (!game_over)
    {

    }

    close(clients[0]);
    close(clients[1]);
    free(clients);

    pthread_exit(NULL);
}

int main (int argc, char* argv[])
{
    if (argc < 2) 
    {
        printf("A port must be specified");
        exit(1);
    }

    int listener_socket = setup_server(atoi(argv[1]));
    int number_connected = 0;

    socklen_t client_len;
    struct sockaddr_in client_address;

    while(1)
    {
        //two player game = two clients at a time
        int* client_sockets = (int*) malloc(2 * sizeof(int*));
        memset(client_sockets, 0, 2 * sizeof(int*));
        while (number_connected < 2)
        {
            listen(listener_socket, 255);
            memset(&client_address, 0, sizeof(client_address));
            client_len = sizeof(client_address);

            client_sockets[number_connected] = accept(listener_socket, (struct sockaddr*) &client_address, &client_len);

            write_status(client_sockets[number_connected], number_connected);

            if (client_sockets[number_connected] < 0)
                handle_error("Cannot accept client connection");
            
            number_connected++;
            if (number_connected == 1)
                write_status(client_sockets[0], WAIT);
        }

        pthread_t thread;
        int result = pthread_create(&thread, NULL, run_game, (void*) client_sockets);
        if (result)
        {
            printf("Failed to create thread, error code %d", result);
            exit(-1);
        }
    }

    close(listener_socket);
}