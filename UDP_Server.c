/* Echo server using UDP */
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_UDP_PORT 2315 /* 2+Last 3 of ID */
#define MAXLEN 4096

void fatal(char *string);

int main(int argc, char **argv)
{
    int sd, client_len, n, loss_prob, protocol;
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
    protocol = atoi(argv[2]) - 1;
    if(protocol < 0 || protocol > 2) fatal("Invalid protocol.");
    /* Create a datagram socket */
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1) fatal("Can't create a socket");

    /* Bind an address to the socket */
    bzero((char *)&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_UDP_PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1) fatal("Can't bind name to socket");
    while (1)
    {
        client_len = sizeof(client);
        if ((n = recvfrom(sd, buf, MAXLEN, 0, (struct sockaddr *)&client, &client_len)) < 0) fatal("Can't receive datagram");

        if (sendto(sd, buf, n, 0, (struct sockaddr *)&client, client_len) != n) fatal("Can't send datagram");
    }
    close(sd);
    return(0);
}

void fatal(char *string)
{
    printf("%s\n", string);
    exit(1);
}
