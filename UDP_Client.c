// UDP Echo Client
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "utils.h"

#define SERVER_UDP_PORT 2315 /* 2+Last 3 of ID */
#define MAXLEN 4096
#define DEFLEN 64

long delay(struct timeval t1, struct timeval t2);
void fatal(char *string);
int frame_count(int data_size, int frame_size);
void usage();

int main(int argc, char **argv)
{
    int frame_size = DEFLEN, port = SERVER_UDP_PORT, written = DATA_LEN;
    int sd, protocol, i;
    char *host, src_filename[MAXLEN];
    FILE *fh;
    struct hostent *hp;
    struct sockaddr_in server, client;

    switch(argc)
    {
        case 7:
            if(strcmp(*++argv, "-s") != 0 || (frame_size = atoi(*++argv)))
            {
                if (frame_size > MAXLEN) fatal("frame_size is too big");
            } /* Drop through */
        case 5:
            protocol = atoi(*++argv);
            host = *++argv;
            break;
        default:
            usage();
    }

    if(protocol < 0 || protocol > 2)
    {
        usage();
    }

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1) fatal("Can't create a socket");
    bzero((char *)&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    hp = gethostbyname(host);
    if (!hp) fatal("Can't get server's IP address");
    bcopy(hp->h_addr, (char *) &server.sin_addr, hp->h_length);
    strcpy(src_filename, *++argv); /* Always src_file at this point */

    /* protocol is always valid, we already checked */
    switch(protocol)
    {
        case 1: /* Stop and wait */
            /* To make life easier, we assume the request makes it there okay */
            saw_send(sd, src_filename, 0, client);
            /* We're always going to fill the buffer unless it's the last packet. */
            fh = fopen(src_filename, "w");
            for(i = 0; written == DATA_LEN; i++)
            {
                written = saw_listen(sd, fh, i, &client, NULL);
            }
            break;
        case 2: /* Go back n */
            fatal("Not implemented.");
            break;
        case 3: /* Selective repeat */
            sr_send(sd, client, NULL, 0, src_filename);
            sr_listen(sd, client, src_filename);
            break;
    }

    close(sd);
    return(0);
}

void fatal(char *string)
{
    printf("%s\n", string);
    exit(1);
}

int frame_count(int data_size, int frame_size)
{
    return (int)ceil((double)data_size/frame_size);
}

void usage()
{
    printf("Usage: ./client_udp [-s frame_size] protocol server-name src-file dest-file\n");
    printf("Protocol values: 1 - Stop and Wait\n");
    printf("                 2 - Go Back N\n");
    fatal("                 3 - Selective Repeat");
}
