#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

#define PORT 9358

#define SID "6413"
#define AUTH_TOKEN "OPS-3146"


/*
 * ---------------------------------------------------------
 * read_line()
 *
 * Reads one complete text line from the TCP socket.
 *
 * TCP is a byte stream, so we keep receiving until '\n'.
 * ---------------------------------------------------------
 */
ssize_t read_line(int fd, char *buffer, size_t max_length)
{
    size_t i = 0;

    while (i < max_length - 1)
    {
        char c;

        ssize_t n = recv(fd, &c, 1, 0);

        if (n == 0)
        {
            /* Agent disconnected */
            return 0;
        }

        if (n < 0)
        {
            perror("recv");
            return -1;
        }

        buffer[i++] = c;

        if (c == '\n')
        {
            break;
        }
    }

    buffer[i] = '\0';

    return (ssize_t)i;
}


/*
 * ---------------------------------------------------------
 * send_all()
 *
 * Sends exactly total_bytes.
 *
 * One send() is NOT guaranteed to send everything.
 * ---------------------------------------------------------
 */
ssize_t send_all(int fd,
                 const void *buffer,
                 size_t total_bytes)
{
    size_t sent = 0;

    while (sent < total_bytes)
    {
        ssize_t n = send(fd,
                         (const char *)buffer + sent,
                         total_bytes - sent,
                         0);

        if (n <= 0)
        {
            perror("send");
            return -1;
        }

        sent += (size_t)n;
    }

    return (ssize_t)sent;
}


/*
 * ---------------------------------------------------------
 * recv_all()
 *
 * Receives exactly total_bytes.
 *
 * This is required for GET because the file size is known.
 * ---------------------------------------------------------
 */
ssize_t recv_all(int fd,
                 void *buffer,
                 size_t total_bytes)
{
    size_t received = 0;

    while (received < total_bytes)
    {
        ssize_t n = recv(fd,
                         (char *)buffer + received,
                         total_bytes - received,
                         0);

        if (n == 0)
        {
            /* Connection closed before all data arrived */
            return (ssize_t)received;
        }

        if (n < 0)
        {
            perror("recv");
            return -1;
        }

        received += (size_t)n;
    }

    return (ssize_t)received;
}


/*
 * ---------------------------------------------------------
 * send_command()
 *
 * Used for normal text commands where the Agent returns
 * exactly one text response line.
 *
 * Used for:
 *     AUTH
 *     SYSINFO
 *     LISTPROC
 *     EXEC
 *     QUIT
 *
 * NOT used for PUT or GET because they contain raw file data.
 * ---------------------------------------------------------
 */
int send_command(int sockfd, const char *command)
{
    char response[8192];

    if (send_all(sockfd,
                 command,
                 strlen(command)) < 0)
    {
        return -1;
    }

    ssize_t n = read_line(sockfd,
                          response,
                          sizeof(response));

    if (n <= 0)
    {
        printf("Agent disconnected.\n");
        return -1;
    }

    printf("Agent: %s", response);

    return 0;
}

/*
 * ---------------------------------------------------------
 * UDP MONITORING
 * ---------------------------------------------------------
 */

typedef struct
{
    int udp_fd;
    volatile int running;
} udp_monitor_t;


void *udp_monitor_receiver(void *arg)
{
    udp_monitor_t *monitor = (udp_monitor_t *)arg;

    char buffer[256];

    struct sockaddr_in agent_addr;
    socklen_t agent_addr_len = sizeof(agent_addr);

    while (monitor->running)
    {
        ssize_t n = recvfrom(monitor->udp_fd,
                             buffer,
                             sizeof(buffer) - 1,
                             0,
                             (struct sockaddr *)&agent_addr,
                             &agent_addr_len);

        if (n > 0)
        {
            buffer[n] = '\0';

            printf("UDP Monitor: %s\n", buffer);
        }
        else if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }

            if (monitor->running)
            {
                perror("recvfrom");
            }

            break;
        }
    }

    return NULL;
}


/*
 * ---------------------------------------------------------
 * main()
 * ---------------------------------------------------------
 */
int main(void)
{
    int sockfd;

    struct sockaddr_in server_addr;

    udp_monitor_t monitor;
    pthread_t monitor_thread;
    


    /*
     * -----------------------------------------------------
     * 1. Create TCP socket
     * -----------------------------------------------------
     */

    sockfd = socket(AF_INET,
                    SOCK_STREAM,
                    0);

    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }


    /*
     * -----------------------------------------------------
     * 2. Configure Agent address
     * -----------------------------------------------------
     */

    memset(&server_addr,
           0,
           sizeof(server_addr));

    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(PORT);


    if (inet_pton(AF_INET,
                  "127.0.0.1",
                  &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");

        close(sockfd);

        exit(EXIT_FAILURE);
    }


    /*
     * -----------------------------------------------------
     * 3. Connect to Agent
     * -----------------------------------------------------
     */

    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect");

        close(sockfd);

        exit(EXIT_FAILURE);
    }

    printf("Connected to Agent!\n");


    /*
     * =====================================================
     * AUTHENTICATION
     * =====================================================
     */

    char auth_message[128];

    snprintf(auth_message,
             sizeof(auth_message),
             "AUTH %s\n",
             AUTH_TOKEN);

    if (send_command(sockfd,
                     auth_message) < 0)
    {
        close(sockfd);
        return 1;
    }


   
        /*
    * =====================================================
    * SYSINFO
    * =====================================================
    */

    printf("\n--- SYSINFO ---\n");

    if (send_command(sockfd, "SYSINFO\n") < 0)
    {
        printf("SYSINFO failed.\n");
        close(sockfd);
        return 1;
    }


    /*
    * =====================================================
    * LISTPROC
    * =====================================================
    */

    printf("\n--- LISTPROC ---\n");

    if (send_command(sockfd, "LISTPROC\n") < 0)
    {
        printf("LISTPROC failed.\n");
        close(sockfd);
        return 1;
    }


    /*
    * =====================================================
    * EXEC
    * =====================================================
    */

    printf("\n--- EXEC ---\n");

    if (send_command(sockfd, "EXEC DATE\n") < 0)
    {
        printf("EXEC failed.\n");
        close(sockfd);
        return 1;
    }

    

    /*
     * =====================================================
     * PUT
     *
     * Upload upload.txt to the Agent.
     * =====================================================
     */

    const char *upload_filename = "upload.txt";

    FILE *upload_file =
        fopen(upload_filename, "rb");

    if (upload_file == NULL)
    {
        perror("fopen upload.txt");

        close(sockfd);

        return 1;
    }


    /*
     * Find the file size.
     */

    if (fseek(upload_file, 0, SEEK_END) != 0)
    {
        perror("fseek");

        fclose(upload_file);
        close(sockfd);

        return 1;
    }

    long upload_filesize = ftell(upload_file);

    if (upload_filesize < 0)
    {
        perror("ftell");

        fclose(upload_file);
        close(sockfd);

        return 1;
    }

    rewind(upload_file);


    /*
     * Build:
     *
     * PUT upload.txt <filesize>\n
     */

    char put_command[512];

    snprintf(put_command,
             sizeof(put_command),
             "PUT %s %ld\n",
             upload_filename,
             upload_filesize);


    /*
     * Send PUT command.
     */

    if (send_all(sockfd,
                 put_command,
                 strlen(put_command)) < 0)
    {
        fclose(upload_file);
        close(sockfd);

        return 1;
    }


    /*
     * Send the actual file bytes.
     */

    char upload_buffer[4096];

    size_t bytes_read;

    while ((bytes_read =
                fread(upload_buffer,
                      1,
                      sizeof(upload_buffer),
                      upload_file)) > 0)
    {
        if (send_all(sockfd,
                     upload_buffer,
                     bytes_read) < 0)
        {
            fclose(upload_file);
            close(sockfd);

            return 1;
        }
    }


    /*
     * Check for file-reading error.
     */

    if (ferror(upload_file))
    {
        perror("fread");

        fclose(upload_file);
        close(sockfd);

        return 1;
    }

    fclose(upload_file);


    /*
     * Receive PUT response.
     */

    char put_response[1024];

    ssize_t put_response_length =
        read_line(sockfd,
                  put_response,
                  sizeof(put_response));

    if (put_response_length <= 0)
    {
        printf("Agent disconnected.\n");

        close(sockfd);

        return 1;
    }

    printf("Agent: %s",
           put_response);


    /*
     * =====================================================
     * GET
     *
     * Download upload.txt from the Agent.
     * =====================================================
    

    const char *get_filename = "upload.txt";

    char get_command[512];

    snprintf(get_command,
             sizeof(get_command),
             "GET %s\n",
             get_filename);


    

    if (send_all(sockfd,
                 get_command,
                 strlen(get_command)) < 0)
    {
        close(sockfd);

        return 1;
    }


    

    char header[1024];

    ssize_t header_length =
        read_line(sockfd,
                  header,
                  sizeof(header));

    if (header_length <= 0)
    {
        printf("Agent disconnected.\n");

        close(sockfd);

        return 1;
    }

    printf("Agent: %s",
           header);


    

    char received_filename[256];

    long filesize;

    if (sscanf(header,
               "OK FILE_SEND %255s %ld",
               received_filename,
               &filesize) != 2)
    {
        printf("Invalid GET response.\n");

        close(sockfd);

        return 1;
    }


    if (filesize < 0)
    {
        printf("Invalid file size.\n");

        close(sockfd);

        return 1;
    }


    

    FILE *downloaded_file =
        fopen("downloaded_upload.txt", "wb");

    if (downloaded_file == NULL)
    {
        perror("fopen downloaded_upload.txt");

        close(sockfd);

        return 1;
    }


    

    char download_buffer[4096];

    long remaining = filesize;

    while (remaining > 0)
    {
        size_t chunk_size;

        if (remaining >
            (long)sizeof(download_buffer))
        {
            chunk_size =
                sizeof(download_buffer);
        }
        else
        {
            chunk_size =
                (size_t)remaining;
        }


        ssize_t received =
            recv_all(sockfd,
                     download_buffer,
                     chunk_size);

        if (received !=
            (ssize_t)chunk_size)
        {
            printf("File transfer failed.\n");

            fclose(downloaded_file);
            close(sockfd);

            return 1;
        }


        size_t written =
            fwrite(download_buffer,
                   1,
                   chunk_size,
                   downloaded_file);

        if (written != chunk_size)
        {
            perror("fwrite");

            fclose(downloaded_file);
            close(sockfd);

            return 1;
        }


        remaining -=
            (long)chunk_size;
    }


    fclose(downloaded_file);


    printf("File downloaded successfully: "
           "downloaded_upload.txt\n");




   

printf("UDP monitoring active for 10 seconds...\n");
sleep(10);



if (send_command(sockfd,
                 "MONITOR STOP\n") < 0)
{
    monitor.running = 0;
    pthread_join(monitor_thread, NULL);
    close(monitor.udp_fd);
    close(sockfd);
    return 1;
}



monitor.running = 0;

pthread_join(monitor_thread, NULL);

close(monitor.udp_fd);



printf("UDP monitoring stopped.\n"); */


        /*
    * =====================================================
    * UDP MONITORING
    * =====================================================
    */

    int udp_port = 9001;

    struct sockaddr_in udp_addr;

    monitor.udp_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (monitor.udp_fd < 0)
    {
        perror("UDP socket");
        close(sockfd);
        return 1;
    }

    memset(&udp_addr, 0, sizeof(udp_addr));

    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_addr.sin_port = htons(udp_port);

    if (bind(monitor.udp_fd,
            (struct sockaddr *)&udp_addr,
            sizeof(udp_addr)) < 0)
    {
        perror("UDP bind");
        close(monitor.udp_fd);
        close(sockfd);
        return 1;
    }

    /*
    * Set a receive timeout so the UDP thread
    * can periodically check the running flag.
    */

    struct timeval timeout;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(monitor.udp_fd,
                SOL_SOCKET,
                SO_RCVTIMEO,
                &timeout,
                sizeof(timeout)) < 0)
    {
        perror("setsockopt");
        close(monitor.udp_fd);
        close(sockfd);
        return 1;
    }

    monitor.running = 1;

    /*
    * Start UDP receiver thread.
    */

    if (pthread_create(&monitor_thread,
                    NULL,
                    udp_monitor_receiver,
                    &monitor) != 0)
    {
        perror("pthread_create");
        close(monitor.udp_fd);
        close(sockfd);
        return 1;
    }

    

    /*
    * Tell the Agent to start sending
    * UDP monitoring packets.
    */

    char monitor_command[128];

    snprintf(monitor_command,
            sizeof(monitor_command),
            "MONITOR START %d\n",
            udp_port);

    if (send_command(sockfd, monitor_command) < 0)
    {
        monitor.running = 0;
        pthread_join(monitor_thread, NULL);
        close(monitor.udp_fd);
        close(sockfd);
        return 1;
    }
        

    /*
 * =====================================================
 * STOP UDP MONITORING
 * =====================================================
 */

printf("UDP monitoring active for 10 seconds...\n");

sleep(10);

if (send_command(sockfd, "MONITOR STOP\n") < 0)
{
    monitor.running = 0;
    pthread_join(monitor_thread, NULL);
    close(monitor.udp_fd);
    close(sockfd);
    return 1;
}

monitor.running = 0;

pthread_join(monitor_thread, NULL);

close(monitor.udp_fd);

printf("UDP monitoring stopped.\n");


/*
 * =====================================================
 * QUIT
 * =====================================================
 */

if (send_command(sockfd, "QUIT\n") < 0)
{
    close(sockfd);
    return 1;
}


    /*
     * =====================================================
     * CLOSE
     * =====================================================
     */

    close(sockfd);

    return 0;
}
