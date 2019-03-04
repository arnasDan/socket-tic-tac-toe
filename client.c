#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "types.h"

void handle_error(const char* message)
{
    printf("An error occured: %s", message);
    exit(0);
}

void draw_board(char board[][3])
{
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
}

int connect_to_server(char* hostname, int port)
{
    struct sockaddr_in server_address;
    struct hostent *server;
 
    int socket_no = socket(AF_INET, SOCK_STREAM, 0);
	
    if (socket_no < 0) 
        handle_error("ERROR opening socket for server.");
	
    server = gethostbyname(hostname);
	
    if (server == NULL)
        handle_error("No such host");
	
	memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    memmove(server->h_addr, &server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port); 

    if (connect(socket_no, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) 
        handle_error("Cannot connect to server");
    
    return socket_no;
}

int receive_int(int socket)
{
    int message;
    int result = recv(socket, &message, sizeof(message), 0);
    switch(result)
    {
        case -1:
            handle_error("Cannot read from server socket");
            break;
        case 0:
            handle_error("Connection closed");
            break;
        default:
            return message;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s host port\n", argv[0]);
        exit(0);
    }
    
    int server_socket = connect_to_server(argv[1], atoi(argv[2]));

    char board[3][3] = { {' ', ' ', ' '},
                         {' ', ' ', ' '}, 
                         {' ', ' ', ' '} };
    
    printf("Tic-Tac-Toe\n");

    int id = receive_int(server_socket);

    while(1)
    {
        int message = receive_int(server_socket);
        switch(message)
        {
            case WAIT:
                printf("Waiting for second player to join...\n");
                break;
            case START:
                printf("Game is starting!\n");
                printf("You are %c's\n", id ? 'X' : 'O');
                draw_board(board);
                break;
            default:
                handle_error("Unknown server signal");
        }
    }


}