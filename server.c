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
    printf("An error occured: %s\n", message);
    pthread_exit(NULL);
}

void write_status(int client_socket, int status_code)
{
    int result = write(client_socket, &status_code, sizeof(int));
    if (result < 0)
    {
        printf("Message was: %d\n", status_code);
        handle_error("Cannot write status to client socket");
    }
        

}

void write_to_clients(int* clients, int status_code)
{
    write_status(clients[0], status_code);
    write_status(clients[1], status_code);
}

int receive_int(int socket)
{
    int message;
    recv(socket, &message, sizeof(int), 0);
    return message;
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
    
    printf("Now listening on %d\n", port);
    return socket_no;
}

int handle_move(char board[][3], int move, int player)
{
    board[move / 3][move % 3] = player ? 'X' : 'O';
    int row = move / 3;
    int col = move % 3;
    printf("Player %d made move %d (row: %d, col: %d) \n", player, move, row, col);
    printf("Board after move:\n");
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
            if (board[i][j] != ' ')
                printf("%c ", board[i][j]);
            else
                printf("%c ", '-');
        printf("\n");
    }
    int row_win =
        board[row][0] == board[row][1] &&
        board[row][0] == board[row][2];
    int column_win =
        board[0][col] == board[1][col] &&
        board[0][col] == board[2][col];
    int diagonal_win =
        (
            (board[0][0] == board[1][1] && board[0][0] == board[2][2] && board[0][0] != ' ') ||
            (board[1][1] == board[0][2] && board[1][1] == board[2][0] && board[1][1] != ' ')
        );
    printf("row_win=%d column_win=%d, diagonal_win=%d\n", row_win, column_win, diagonal_win);
    return row_win || column_win || diagonal_win;
}

void* run_game(void* thread_context)
{
    int* clients = (int*) thread_context;

    int game_over = 0, player_turn = 1, turn_count = 0, move;

    char board[3][3] = { {' ', ' ', ' '},
                        {' ', ' ', ' '}, 
                        {' ', ' ', ' '} };
    
    write_to_clients(clients, START);

    while (!game_over && turn_count <= 8)
    {
        write_status(clients[!player_turn], NOT_YOUR_TURN);
        write_status(clients[player_turn], TURN);
        
        //loop until valid move is provided
        while (1)
        {
            move = receive_int(clients[player_turn]);
            if (move == -1 || (move >= 0 && move <= 8 && board[move / 3][move % 3] == ' '))
                break;
            else
                write_status(clients[player_turn], INVALID);
        }
        if (move == -1)
            break;
        
        //inform about move
        write_to_clients(clients, UPDATE);
        write_to_clients(clients, player_turn);
        write_to_clients(clients, move);

        turn_count++;
        
        game_over = handle_move(board, move, player_turn);

        player_turn = !player_turn;
    }

    if (game_over && move != -1)
    {
        write_status(clients[!player_turn], WON);
        write_status(clients[player_turn], LOST);
    }
    else if (turn_count == 8)
        write_to_clients(clients, DRAW);
    
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
        number_connected = 0;
    }

    close(listener_socket);
    return 0;
}