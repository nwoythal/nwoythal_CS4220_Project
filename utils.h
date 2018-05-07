#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

#define DATA_LEN 4096
#define TIMEOUT  2000
#define str(s) #s

struct frame
{
    int sequence_no;
    int length;
    char body[DATA_LEN];
};

/***********************
 *  SELECTIVE REPEAT   *
 ***********************/
int sr_send(int sock_handle, struct sockaddr_in client, int drop_chance,
            FILE *fh)
{
    struct frame data_frame, nack;
    struct timeval timeout;
    char buf[DATA_LEN];

    timeout.tv_usec = 100;
    setsockopt(sock_handle, SOL_SOCKET, SO_RCVTIMEO, &timeout, 
                sizeof(timeout));

    while(1)
    {
        memset(data_frame.body, 0, sizeof(data_frame.body));
        if(fread(buf, sizeof(char), DATA_LEN, fh) > 0)
        {
            sendto(sock_handle, &data_frame, sizeof(data_frame), 0, 
                    (struct sockaddr *)&client, sizeof(client));
        }

        /* Listen passively for .1 ms */
        recvfrom(sock_handle, &nack, sizeof(nack), 0,
                            (struct sockaddr*) &client, 
                            (socklen_t *)sizeof(client));

        if(nack.sequence_no == -1)
        {
            break;
        }
        else
        {
            fseek(fh, DATA_LEN * nack.sequence_no, SEEK_SET);
            sendto(sock_handle, &data_frame, sizeof(data_frame), 0, 
                    (struct sockaddr *)&client, sizeof(client));
            fseek(fh, 0, SEEK_END);
        }
    }
    return 0;
}

int sr_listen(int sock_handle, struct sockaddr_in client, int drop_chance,
                char *filename)
{
    int i, j, dropped_packet, last_packet, offset;
    struct frame data_frame, nack;
    FILE *fh = fopen(filename, "r+b");
    int dropped[8];

    memset(dropped, -1, sizeof(dropped));

    printf("BEGINNING SR_LISTEN\n");
    /* Passively listen. Stupid hack to determine last packet. */
    while(data_frame.body[DATA_LEN - 1] != '\0')
    {
        recvfrom(sock_handle, &data_frame, sizeof(data_frame), 0,
                            (struct sockaddr*) &client, 
                            (socklen_t *)sizeof(client));
        dropped_packet = (data_frame.sequence_no != (last_packet + 1));
        if(!dropped_packet)
        {
            last_packet = data_frame.sequence_no;
        }
        /* Queue management */
        for(i = 0; i < 8; i++)
        {
            /* Iterate over the queue, see if it's a packet we missed */
            if(data_frame.sequence_no == dropped[i])
            {
                dropped[i] = -1;
                /* To avoid sync errors, we need to calculate offset. */
                offset = data_frame.sequence_no * DATA_LEN;
                for(j = 0; j < 8; j++)
                {
                    if(dropped[j] < data_frame.sequence_no)
                    {
                        offset -= DATA_LEN;
                    }
                }
                fseek(fh, offset, SEEK_SET);
                fputs(data_frame.body, fh);
                fseek(fh, 0, SEEK_END);
            }
            /* Check to see if we missed a packet. Use first fit. */
            if(dropped_packet && dropped[i] == -1)
            {
                dropped_packet = 0;
                dropped[i] = data_frame.sequence_no;
                nack.sequence_no = dropped[i];
                sendto(sock_handle, &nack, sizeof(nack), 0, 
                        (struct sockaddr *)&client, sizeof(client));
            }
            /* Check to see if the queue is full. */
            if(dropped_packet && i == 7)
            {
                fclose(fh);
                printf("!!! DROPPED QUEUE IS FILLED !!!\n");
                return -1;
            }
        }
    }
    nack.sequence_no = -1;
    sendto(sock_handle, &nack, sizeof(nack), 0, (struct sockaddr *) &client,
             sizeof(client));

    return 0;
}

/***********************
 *    STOP AND WAIT    *
 ***********************/
int saw_send(int sock_handle, char *file_str, int drop_chance, 
                struct sockaddr_in client)
{ 
    int result, bytes, ack_recieved, i;
    struct frame data_frame, ack;
    struct timeval timeout, del_start, del_end;
    FILE *file_handle = fopen(file_str, "r");
    timeout.tv_sec = TIMEOUT / 1000;
    timeout.tv_usec = 0;
    data_frame.sequence_no = 0;
    
    printf("BEGINNING SAW_SEND\n");

    while(1)
    {
        setsockopt(sock_handle, SOL_SOCKET, SO_RCVTIMEO, &timeout, 
                    sizeof(timeout));
        bytes = fread(data_frame.body, sizeof(char), DATA_LEN, file_handle);
        /* Check if the file is completely sent. */
        if(bytes == 0)
        {
            break;
        }
        i = 0;
        ack_recieved = -1;
        do
        {
            gettimeofday(&del_start, NULL);
            /* Do we drop that packet like an ugly baby? */
            if(((rand() % 100) + 1) > drop_chance)
            {
                do
                {
                    printf("ATTEMPTING TO SEND %d BYTES IN PACKET %d\n", 
                            bytes, i);
                    result = sendto(sock_handle, &data_frame,
                                    sizeof(data_frame), 0, 
                                    (struct sockaddr *)&client, 
                                    sizeof(client));
                } while(result == -1);
                data_frame.sequence_no = i++;
                printf("SUCESSFULLY SENT %d BYTES IN PACKET %d\n", bytes, i);
            }
            ack_recieved = recvfrom(sock_handle, &ack, sizeof(ack), 0,
                                    (struct sockaddr*) &client,
                                    (socklen_t *)sizeof(client));
            if(ack_recieved != -1)
            {
                gettimeofday(&del_end, NULL);
                timeout.tv_sec = (del_end.tv_sec - del_start.tv_sec) + 
                                    (del_end.tv_usec - del_start.tv_usec) 
                                    / 1000; 
                printf("RECIEVED ACK %d\n", i);
            }
        } while(ack_recieved == -1 ||
                ack.sequence_no != data_frame.sequence_no);
    }
    fclose(file_handle);
    printf("FINISHING SAW_SEND\n");
    return 0;
}

int saw_listen(int sock_handle, FILE *file_handle, int last_frame_num, 
                struct sockaddr_in *client, char *buf)
{
    struct frame data_frame, ack;
    int written;
    /* Listen for packet. */
    printf("BEGINNING SAW_LISTEN\n");
    do
    {
        recvfrom(sock_handle, &data_frame, sizeof(data_frame), 0,
                (struct sockaddr*) &client, (socklen_t *)sizeof(client));
    } while(data_frame.sequence_no != (last_frame_num + 1));
    printf("RECIEVED PACKET %d, SENDING ACK %d\n",
            data_frame.sequence_no, data_frame.sequence_no);

    /* Got the expected packet, send the ACK */
    ack.sequence_no = data_frame.sequence_no;
    sendto(sock_handle, &ack, sizeof(ack), 0, (struct sockaddr *) &client, 
            sizeof(client));

    printf("ACK %d SENT", data_frame.sequence_no);
    if(buf != NULL)
    {
        buf = data_frame.body; 
        written = sizeof(buf);
    }
    else
    {
        written = fwrite(data_frame.body, sizeof(char),sizeof(data_frame.body),
            file_handle);
    }
    printf("FINISHING SAW_LISTEN\n");
    return written;
}

