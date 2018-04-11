// UDP Echo Client
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>

#define SERVER_UDP_PORT 2315 /* 2+Last 3 of ID */
#define MAXLEN 4096
#define DEFLEN 64

long delay(struct timeval t1, struct timeval t2);
void fatal(char *string);
int frame_count(int data_size, int frame_size);

int main(int argc, char **argv)
{
    int frame_size = DEFLEN, port = SERVER_UDP_PORT;
    int sd, server_len, num_frames;
    char *host, rbuf[MAXLEN], sbuf[MAXLEN], src_filename[MAXLEN];
    struct hostent *hp;
    struct sockaddr_in server;
    struct timeval start, end;

    argv++;
    switch(argc)
    {
        case 6:
            if(strcmp(*argv, "-s") != 0 || (frame_size = atoi(*++argv)))
            {
                if (frame_size > MAXLEN) fatal("Data is too big");
                argv++;
            } /* Drop through */
        case 4:
            host = *argv;
            argv++;
            break;
        default:
            fatal("Usage: ./client_udp [-s frame_size] server-name src-file dest-file ");
    }

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1) fatal("Can't create a socket");
    bzero((char *)&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    hp = gethostbyname(host);
    if (!hp) fatal("Can't get server's IP address");
    bcopy(hp->h_addr, (char *) &server.sin_addr, hp->h_length);
    strcpy(src_filename, *argv);
    num_frames = frame_count(strlen(src_filename), frame_size);

    /* construct data to send to the server */
    gettimeofday(&start, NULL); /* start delay measurement */
    server_len = sizeof(server);
    if (sendto(sd, sbuf, frame_size, 0, (struct sockaddr *)&server, server_len) == -1) fatal("sendto error");
    if (recvfrom(sd, rbuf, MAXLEN, 0, (struct sockaddr *)&server, &server_len) < 0) fatal("recvfrom error");
    gettimeofday(&end, NULL); /* end delay measurement */

    if (strncmp(sbuf, rbuf, frame_size) != 0)
    {
        printf("Data is corrupted\n");
    }

    close(sd);
    return(0);
}

long delay(struct timeval t1, struct timeval t2)
{
    long d;
    d = (t2.tv_sec - t1.tv_sec) * 1000;
    d += ((t2.tv_usec - t1.tv_usec + 500) / 1000);
    return(d);
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
