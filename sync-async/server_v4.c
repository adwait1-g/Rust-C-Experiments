/*
 * server_v1.c
 * 
 * Simple I/O multiplexing using select.
 * - Server simply receives data and sends some data to client.
 * - server_v5.c extends on this concept and an echo server is written.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>

void serve_connection (int client_fd)
{   
    uint8_t         request_buffer[10000] = {0};
    uint8_t         response_buffer[10000] = {0};
    int             ret = 0;

    // Only one request response!
    ret = recv(client_fd, request_buffer, sizeof(request_buffer), 0);
    if (ret < 0)
    {
        printf("recv() failed for fd = %d\n", ret);
        return;
    }

    // Print the request (symbolic of processing the request)
    printf("%d: %s\n", client_fd, request_buffer);

    // Send back response
    ret = send(client_fd, "Hello from server!", 19, 0);
    printf("send() for descriptor %d return %d\n", client_fd, ret);
    if (ret < 19)
    {
        printf("send() failed for fd = %d\n", ret);
        return;
    }

    // Done, go back.
    return;
}


int main (int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: $ %s [host-ipv4-address] [port-number]\n", argv[0]);
        return 0;
    }

    int                 sock_fd = 0;
    int                 client_fd = 0;
    int                 ret = 0;
    int                 i = 0;
    struct sockaddr_in  server_addr = {0};
    struct sockaddr_in  client_addr = {0};
    socklen_t           client_addr_len = 0;
    const char          *ip_addr = argv[1];
    uint16_t            port_no = atoi(argv[2]);
    fd_set              read_set = {0};
    bool                fds_list[FD_SETSIZE] = {0};

    // Lets create a socket.
    ret = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (ret < 0)
    {
        printf("socket() failed\n");
        return -1;
    }
    sock_fd = ret;

    // Bind the socket to the passed (ip_address, port_no).
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    ret = bind(sock_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        printf("bind() failed\n");
        return -1;
    }
    
    // Start listening
    ret = listen(sock_fd, 50);
    if (ret < 0)
    {
        printf("listen() failed\n");
        return -1;
    }
    printf("Listening at (%s, %u)\n", ip_addr, port_no);

    // Before entering, initialize everything we need to call select.
    // These sets are passed to select, they get altered when select succeeds.
    FD_ZERO(&read_set);

    // Do the thing
    while (1)
    {
        memset(&client_addr, '\0', sizeof(client_addr));

        // Copy.
        FD_SET(sock_fd, &read_set);

        printf("Waiting for select() to succeed\n");
        ret = select(FD_SETSIZE, &read_set, NULL, NULL, NULL);
        if (ret < 0)
        {
            printf("select() failed\n");
            return -1;
        }
        printf("After select!\n");
        // Suppose it succeeds, we don't know which sockets are ready.
        // All the "ready" sockets are set in those sets.
        // We know the socket value, we need to go over them and see
        // which one to process first.
        
        // If this comes true, that means we have an incoming connection
        // waiting to be accepted.
        if (FD_ISSET(sock_fd, &read_set))
        {
            ret = accept4(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len, SOCK_NONBLOCK);
            if (ret < 0)
            {
                printf("accept4() failed\n");
                return -1;
            }
            client_fd = ret;
            printf("client_fd = %d\n", client_fd);

            // We need to store this descriptor in our array.
            // If a descriptor(say 5) is in use, the kernel will never
            // return 5 as the descriptor for another socket/file.
            // So, we can go ahead and save it.
            fds_list[client_fd] = true;
            FD_SET(client_fd, &read_set);
        }
        else // This should cover all the other socket descriptors
        {
            // Should we iterate through our fds_list and check
            // which descriptor is present in the read/write/error sets?
            for (i = 0; i < FD_SETSIZE; i++)
            {   
                // If that socket exists
                if (fds_list[i] == true)
                {   
                    printf("FD: %d\n", i);
                    // Can we read from it?
                    // Did we receive any data at this descriptor?
                    if (FD_ISSET(i, &read_set))
                    {   
                        // Read that data, and send back a response
                        serve_connection(i);
                        close(i);
                        FD_CLR(i, &read_set);
                        fds_list[i] = false;
                    }
                }
            }
        }


        i += 1;
    }
}