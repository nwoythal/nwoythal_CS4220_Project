#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

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
int sr_send()
{
    return 0;
}

int sr_listen()
{
    return 0;
}

/***********************
 *    STOP AND WAIT    *
 ***********************/
int saw_send(int sock_handle, char *file_str, int drop_chance, 
                struct sockaddr_in client)
{ 
    int result, bytes, ack_recieved;
    struct frame data_frame, ack;
    struct timeval timeout, del_start, del_end;
    FILE *file_handle = fopen(file_str, "r");
    timeout.tv_sec = TIMEOUT / 1000;
    timeout.tv_usec = 0;
    data_frame.sequence_no = 0;

    while(1)
    {
        result = setsockopt(sock_handle, SOL_SOCKET, SO_RCVTIMEO, &timeout, 
                             sizeof(timeout));
        bytes = fread(data_frame.body, sizeof(char), DATA_LEN, file_handle) ;
        /* Check if the file is completely sent. */
        if(bytes == 0)
        {
            break;
        }

        /* Loop until we get the ACK. */
        ack_recieved = -1;
        do
        {
            gettimeofday(&del_start, NULL);
            /* Do we drop that packet like an ugly baby? */
            if(((rand() % 100) + 1) > drop_chance)
            {
                do
                {
                    result = sendto(sock_handle, &data_frame,
                                    sizeof(data_frame), 0, 
                                    (struct sockaddr *)&client, 
                                    sizeof(client));
                } while(result == -1);
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
            }
        } while(ack_recieved == -1 ||
                ack.sequence_no != data_frame.sequence_no);
    }
    fclose(file_handle);
    return 0;
}

int saw_listen(int sock_handle, FILE *file_handle, int last_frame_num, 
                struct sockaddr_in *client, char *buf)
{
    struct frame data_frame, ack;
    int written;
    /* Listen for packet. */
    do
    {
        recvfrom(sock_handle, &data_frame, sizeof(data_frame), 0,
                (struct sockaddr*) &client, (socklen_t *)sizeof(client));
    } while(data_frame.sequence_no != (last_frame_num + 1));

    /* Got the expected packet, send the ACK */
    ack.sequence_no = data_frame.sequence_no;
    sendto(sock_handle, &ack, sizeof(ack), 0, (struct sockaddr *) &client, 
            sizeof(client));

    if(buf != NULL)
    {
        buf = data_frame.body;    
        written = sizeof(buf);
    }
    else
    {
        written = fwrite(data_frame.body, sizeof(char), sizeof(data_frame.body),
            file_handle);
    }
    return written;
}

