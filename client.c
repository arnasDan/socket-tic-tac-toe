#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "types.h"

#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0501
     #endif
    #include <winsock2.h>
    #include <Ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

void handle_error(const char* message)
{
    fprintf(stderr, "An error occured: %s\n", message);
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

    #ifdef _WIN32
        if (socket_no == INVALID_SOCKET)
            handle_error("Cannot open socket for listening");
    #else
        if (socket_no < 0)
            handle_error("Cannot open socket for listening");
    #endif

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
    if (result == -1 || result != sizeof(int))
        handle_error("Cannot read from server socket");
    else if (result == 0)
        handle_error("Connection closed");
    return message;
}

void write_int(int server_socket, int number)
{
    int result = write(server_socket, &number, sizeof(int));
    if (result < 0 || result != sizeof(int))
        handle_error("Cannot write number to server socket");
}

char get_xo(int player)
{
    return player ? 'X' : 'O';
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s host port\n", argv[0]);
        exit(0);
    }

    #ifdef _WIN32
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(1,1), &wsa_data) != 0)
            handle_error("Error initialising winsock");
    #endif

    int server_socket = connect_to_server(argv[1], atoi(argv[2]));

    char board[3][3] = { {' ', ' ', ' '},
                         {' ', ' ', ' '},
                         {' ', ' ', ' '} };

    printf("Tic-Tac-Toe\n");

    int id = receive_int(server_socket);

    int game_over = 0, player, move;
    while (!game_over)
    {
        int message = receive_int(server_socket);
        switch(message)
        {
            case WAIT:
                printf("Waiting for second player to join...\n");
                break;
            case NOT_YOUR_TURN:
                printf("Wait for your opponent to make a turn\n");
                break;
            case START:
                printf("Game is starting!\n");
                printf("You are %c's\n", get_xo(id));
                draw_board(board);
                break;
            case UPDATE:
                player = receive_int(server_socket);
                move = receive_int(server_socket);
                board[move / 3][move % 3] = get_xo(player);
                printf("Move made, board afterwards:\n");
                draw_board(board);
                break;
            case INVALID:
                printf("Invalid move!\n");
            case TURN:
                printf("Make your move, 0-8: ");
                int move;
                scanf("%d", &move);
                write_int(server_socket, move);
                break;
            case DRAW:
                printf("Game over. It's a draw!");
                game_over = 1;
                break;
            case WON:
                printf("Congratulations, you've won!");
                game_over = 1;
                break;
            case LOST:
                printf("You've lost :(");
                game_over = 1;
                break;
            default:
                handle_error("Unknown server signal");
        }
    }

    #ifdef _WIN32
        closesocket(server_socket);
    #else
        close(server_socket);
    #endif

    #ifdef _WIN32
        if (WSACleanup() != 0)
            handle_error("Error cleaning up winsock");
    #endif
    return 0;
}
