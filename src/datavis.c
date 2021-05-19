#include <datavis.h>
#include <string.h>
#include <termios.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <main.h>
/**
 * @brief DataVis data structure.
 * 
 */
data_packet g_datavis_st;
/**
 * @brief Mutex to ensure atomicity of DataVis and ACS variable access.
 * 
 */
pthread_mutex_t datavis_mutex;
/**
 * @brief Condition variable used by ACS to signal to DataVis that data is ready.
 * 
 */
pthread_cond_t datavis_drdy;

typedef struct sockaddr sk_sockaddr;

void *datavis_thread(void *t)
{
    int server_fd, new_socket = -1;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        pthread_exit(NULL);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)))
    {
        perror("setsockopt reuseaddr");
        pthread_exit(NULL);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT,
                   &opt, sizeof(opt)))
    {
        perror("setsockopt reuseport");
        pthread_exit(NULL);
    }
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl");
        pthread_exit(NULL);
    }
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        pthread_exit(NULL);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        pthread_exit(NULL);
    }
    memcpy(g_datavis_st.data.start, "FBEGIN", 6);
    memcpy(g_datavis_st.data.end, "FEND", 4);
    while (!done)
    {
        char buf[PACK_SIZE + sizeof(char)];
        pthread_mutex_lock(&datavis_mutex);                                         // wait for wakeup from ACS thread
        memcpy(buf + sizeof(char), g_datavis_st.buf, PACK_SIZE);
        pthread_mutex_unlock(&datavis_mutex); 
        *(char *)buf = PACK_SIZE;
        ssize_t sz = send(new_socket, buf, sizeof(buf), MSG_NOSIGNAL);
        if (sz < 0 && !done)
        {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                     (socklen_t *)&addrlen)) < 0)
            {
#ifdef SERVER_DEBUG
                perror("accept");
#endif
            }
        }
        usleep(1000000 / 10); // 10 Hz
    }
    close(new_socket);
    close(server_fd);
    pthread_exit(NULL);
}