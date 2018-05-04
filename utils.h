#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
int saw_send(int sock_handle, int file_handle, int drop_chance, 
                struct sockaddr_in client)
{ 
    int result, bytes, ack_recieved;
    struct frame data_frame, ack;
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT / 1000;
    timeout.tv_usec = 0;
    data_frame.sequence_no = 0;
    result = setsockopt(sock_handle, SOL_SOCKET, SO_RCVTIMEO, &timeout, 
                        sizeof(timeout));

    if(result < 0)
    {
        return result;
    }
    while(1)
    {
        bytes = read(file_handle, data_frame.body, DATA_LEN);
        /* Check if the file is completely sent. */
        if(bytes == 0)
        {
            break;
        }

        /* Loop until we get the ACK. */
        ack_recieved = -1;
        do
        {
            /* Do we drop that packet like an ugly baby? */
            if(((rand() % 100) + 1) > drop_chance)
            {
                result = sendto(sock_handle, &data_frame, sizeof(data_frame),
                                0, (struct sockaddr *)&client, sizeof(client));
            }
            ack_recieved = recvfrom(sock_handle, &ack, sizeof(ack), 0,
                                    (struct sockaddr*) &client,
                                    (socklen_t *)sizeof(client));
        } while(ack_recieved == -1 ||
                ack.sequence_no != data_frame.sequence_no);
    }
    return 0;
}

int saw_listen()
{
    return 0;
}
