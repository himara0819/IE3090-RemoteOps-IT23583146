#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

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
 * main()
 * ---------------------------------------------------------
 */
int main(void)
{
    int sockfd;

    struct sockaddr_in server_addr;


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


    /*
     * =====================================================
     * LISTPROC
     * =====================================================
     */



    /*
     * =====================================================
     * EXEC
     * =====================================================
     */



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
     */

    const char *get_filename = "upload.txt";

    char get_command[512];

    snprintf(get_command,
             sizeof(get_command),
             "GET %s\n",
             get_filename);


    /*
     * Send GET command.
     */

    if (send_all(sockfd,
                 get_command,
                 strlen(get_command)) < 0)
    {
        close(sockfd);

        return 1;
    }


    /*
     * Receive:
     *
     * OK FILE_SEND <filename> <filesize> SID:<sid>\n
     */

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


    /*
     * Extract filename and filesize.
     */

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


    /*
     * Create local downloaded file.
     */

    FILE *downloaded_file =
        fopen("downloaded_upload.txt", "wb");

    if (downloaded_file == NULL)
    {
        perror("fopen downloaded_upload.txt");

        close(sockfd);

        return 1;
    }


    /*
     * Receive exactly 'filesize' bytes.
     */

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


    /*
     * =====================================================
     * QUIT
     * =====================================================
     */

    if (send_command(sockfd,
                     "QUIT\n") < 0)
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
