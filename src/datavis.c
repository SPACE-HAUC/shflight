#include <datavis.h>
#include <shflight_externs.h>

void *datavis_thread(void *t)
{
    int server_fd, new_socket; //, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        //exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port PORT
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)))
    {
        perror("setsockopt");
        //exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                   sizeof(timeout)) < 0)
        perror("setsockopt failed\n");

    if (setsockopt(server_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                   sizeof(timeout)) < 0)
        perror("setsockopt failed\n");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port PORT
    if (bind(server_fd, (sk_sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        // exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 32) < 0) // listen for connections, allow up to 32 connections
    {
        perror("listen");
        // exit(EXIT_FAILURE);
    }
    while (!done)
    {
        pthread_cond_wait(&datavis_drdy, &datavis_mutex);                                         // wait for wakeup from ACS thread
        if ((new_socket = accept(server_fd, (sk_sockaddr *)&address, (socklen_t *)&addrlen)) < 0) // accept incoming connection
        {
            // perror("accept");
        }
        ssize_t numsent = send(new_socket, &g_datavis_st.buf, PACK_SIZE, 0); // send data
        if (numsent > 0 && numsent != PACK_SIZE)                         // if the whole packet was not sent, print error in console
        {
            perror("DataVis: Send: ");
        }
        //valread = read(sock,recv_buf,32);
        close(new_socket);
        //sleep(1);
    }
    close(server_fd);
    pthread_exit(NULL);
}