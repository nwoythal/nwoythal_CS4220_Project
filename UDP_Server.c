/* Echo server using UDP */
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "utils.h"

#define SERVER_UDP_PORT 2315 /* 2+Last 3 of ID */
#define MAXLEN 4096

int sd; /* Global handle so we can close on ctrl+c */

void fatal(char *string);
void send_data(int protocol, int frames_needed, int socket);
void handle_sigint();

int main(int argc, char **argv)
{
    int loss_prob, protocol, get_request;
    char buf[MAXLEN];
    struct sockaddr_in server, client;

    if(argc != 3)
    {
        fprintf(stderr, "Usage: ./udp_server [loss_probabilty] [protocol_type]\n");
        fprintf(stderr, "    Protocol types: 1 - Stop and wait\n");
        fprintf(stderr, "                    2 - Go back N\n");
        fatal("                    3 - Selective repeat\n");
    }
    loss_prob = atoi(argv[1]);
    if(loss_prob < 0 || loss_prob > 100) fatal("Invalid loss probability.");
    protocol = atoi(argv[2]);
    if(protocol < 1 || protocol > 3) fatal("Invalid protocol.");
    /* Create a datagram socket */
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1) fatal("Can't create a socket");

    signal(SIGINT, handle_sigint); /* Create signal handler */

    /* Bind an address to the socket */
    bzero((char *)&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_UDP_PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) fatal("Can't bind name to socket");
    while(1)
    {
        get_request = saw_listen(sd, NULL, 0, &client, buf);
        if(get_request > 0)
        {
            saw_send(sd, buf, loss_prob, client);
        }
    }
}

void fatal(char *string)
{
    printf("%s\n", string);
    exit(1);
}

void handle_sigint()
{
    close(sd);
    exit(0);
}
