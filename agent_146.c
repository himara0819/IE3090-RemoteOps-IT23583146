#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#define PORT 9358

#define SID "6413"
#define AUTH_TOKEN "OPS-3146"
/*read_line function*/
ssize_t read_line(int fd, char *buffer, size_t max_length)
{
    size_t i = 0;

    while (i < max_length - 1)
    {
        char c;

        ssize_t n = recv(fd, &c, 1, 0);

        if (n == 0)
        {
            /* Client disconnected */
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

    return i;
}

/*recv_all function*/
ssize_t recv_all(int fd, void *buffer, size_t total_bytes)
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
            return received;
        }

        if (n < 0)
        {
            perror("recv");
            return -1;
        }

        received += n;
    }

    return received;
}

/*send_all*/
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

        sent += n;
    }

    return sent;
}

/*get_sysinfo function*/
void get_sysinfo(double *cpu_load,
                 long *mem_used_mb,
                 long *uptime_sec)
{
    FILE *file;

    /* ---------------- CPU LOAD ---------------- */

    file = fopen("/proc/loadavg", "r");

    if (file != NULL)
    {
        fscanf(file, "%lf", cpu_load);
        fclose(file);
    }
    else
    {
        *cpu_load = 0.0;
    }

    /* ---------------- MEMORY ---------------- */

    long mem_total_kb = 0;
    long mem_available_kb = 0;

    file = fopen("/proc/meminfo", "r");

    if (file != NULL)
    {
        char line[256];

        while (fgets(line, sizeof(line), file))
        {
            if (sscanf(line, "MemTotal: %ld kB",
                       &mem_total_kb) == 1)
            {
                continue;
            }

            if (sscanf(line, "MemAvailable: %ld kB",
                       &mem_available_kb) == 1)
            {
                continue;
            }
        }

        fclose(file);
    }

    *mem_used_mb =
        (mem_total_kb - mem_available_kb) / 1024;

    /* ---------------- UPTIME ---------------- */

    double uptime;

    file = fopen("/proc/uptime", "r");

    if (file != NULL)
    {
        fscanf(file, "%lf", &uptime);
        fclose(file);

        *uptime_sec = (long)uptime;
    }
    else
    {
        *uptime_sec = 0;
    }
}

/*get_processes*/
void get_processes(char *output, size_t output_size)
{
    FILE *pipe;

    pipe = popen("ps -eo pid,comm --no-headers", "r");

    if (pipe == NULL)
    {
        snprintf(output,
                 output_size,
                 "PROCESS_LIST_ERROR");
        return;
    }

    output[0] = '\0';

    char line[128];

    while (fgets(line, sizeof(line), pipe) != NULL)
    {
        if (strlen(output) + strlen(line) + 1 >= output_size)
        {
            break;
        }

        /* Remove newline */
        line[strcspn(line, "\n")] = '\0';

        if (strlen(output) > 0)
        {
            strncat(output, ",", output_size - strlen(output) - 1);
        }

        strncat(output,
                line,
                output_size - strlen(output) - 1);
    }

    pclose(pipe);
}
/*execute command function*/
void execute_command(const char *command,
                     char *output,
                     size_t output_size)
{
    const char *shell_command = NULL;

    if (strcmp(command, "DATE") == 0)
    {
        shell_command = "date";
    }
    else if (strcmp(command, "UPTIME") == 0)
    {
        shell_command = "uptime";
    }
    else if (strcmp(command, "DISKFREE") == 0)
    {
        shell_command = "df -h /";
    }
    else if (strcmp(command, "HOSTNAME") == 0)
    {
        shell_command = "hostname";
    }
    else if (strcmp(command, "WHOAMI") == 0)
    {
        shell_command = "whoami";
    }
    else
    {
        output[0] = '\0';
        return;
    }

    FILE *pipe = popen(shell_command, "r");

    if (pipe == NULL)
    {
        snprintf(output,
                 output_size,
                 "EXECUTION_ERROR");
        return;
    }

    if (fgets(output, output_size, pipe) != NULL)
    {
        output[strcspn(output, "\n")] = '\0';
    }
    else
    {
        snprintf(output,
                 output_size,
                 "NO_OUTPUT");
    }

    pclose(pipe);
}


/*handle_client()*/
void *handle_client(void *arg)
{
    int client_fd = *(int *)arg;

    free(arg);

    printf("Controller connected!\n");

    char buffer[1024];
    int authenticated = 0;

    /*
     * The existing client-handling code will go here.
     */
     while (1)
    {
    ssize_t n = read_line(client_fd, buffer, sizeof(buffer));

    if (n <= 0)
    {
        printf("Controller disconnected.\n");
        break;
    }

    printf("Received: %s", buffer);

    /*
     * Authentication
     */
    if (!authenticated)
    {
        if (strcmp(buffer, "AUTH OPS-3146\n") == 0)
        {
            char response[] =
                "OK AUTHENTICATED SID:6413\n";

            send(client_fd,
                 response,
                 strlen(response),
                 0);

            authenticated = 1;

            printf("Authentication successful. SID:6413\n");
        }
        else
        {
            char response[] =
                "ERR 001 AUTH_FAILED SID:6413\n";

            send(client_fd,
                 response,
                 strlen(response),
                 0);

            printf("Authentication failed.\n");
        }

        continue;
    }

    /*
     * QUIT
     */
    if (strcmp(buffer, "QUIT\n") == 0)
    {
        char response[] =
            "OK BYE SID:6413\n";

        send(client_fd,
             response,
             strlen(response),
             0);

        break;
    }

	/*
 * SYSINFO command
 */
if (strcmp(buffer, "SYSINFO\n") == 0)
{
    double cpu_load;
    long mem_used_mb;
    long uptime_sec;

    get_sysinfo(&cpu_load,
                &mem_used_mb,
                &uptime_sec);

    char response[256];

    snprintf(response,
             sizeof(response),
             "OK SYSINFO %.2f %ld %ld SID:6413\n",
             cpu_load,
             mem_used_mb,
             uptime_sec);

    send(client_fd,
         response,
         strlen(response),
         0);
}
/*LISTPROC command*/
else if (strcmp(buffer, "LISTPROC\n") == 0)
{
    char processes[4096];

    get_processes(processes, sizeof(processes));

    char response[4500];

    snprintf(response,
             sizeof(response),
             "OK PROCS %s SID:6413\n",
             processes);

    send(client_fd,
         response,
         strlen(response),
         0);
}

/*EXEC*/
else if (strncmp(buffer, "EXEC ", 5) == 0)
{
    char command[32];

    sscanf(buffer, "EXEC %31s", command);

    char output[512];

    if (strcmp(command, "DATE") != 0 &&
        strcmp(command, "UPTIME") != 0 &&
        strcmp(command, "DISKFREE") != 0 &&
        strcmp(command, "HOSTNAME") != 0 &&
        strcmp(command, "WHOAMI") != 0)
    {
        char response[] =
            "ERR 002 COMMAND_NOT_ALLOWED SID:6413\n";

        send(client_fd,
             response,
             strlen(response),
             0);
    }
    else
    {
        execute_command(command,
                        output,
                        sizeof(output));

        char response[1024];

        snprintf(response,
                 sizeof(response),
                 "OK EXEC_RESULT %s SID:6413\n",
                 output);

        send(client_fd,
             response,
             strlen(response),
             0);
    }
}

/* =================================================
 * PUT
 * ================================================= */

if (strncmp(buffer, "PUT ", 4) == 0)
{
    char filename[256];
    long filesize;

    if (sscanf(buffer,
               "PUT %255s %ld",
               filename,
               &filesize) != 2)
    {
        char response[128];

        snprintf(response,
                 sizeof(response),
                 "ERR 003 INVALID_PUT SID:%s\n",
                 SID);

        send(client_fd,
             response,
             strlen(response),
             0);

        continue;
    }

    if (filesize < 0)
    {
        char response[128];

        snprintf(response,
                 sizeof(response),
                 "ERR 003 INVALID_PUT SID:%s\n",
                 SID);

        send(client_fd,
             response,
             strlen(response),
             0);

        continue;
    }

    char filepath[512];

    snprintf(filepath,
             sizeof(filepath),
             "./agentfiles/IT23583146/%s",
             filename);

    FILE *file = fopen(filepath, "wb");

    if (file == NULL)
    {
        char response[128];

        snprintf(response,
                 sizeof(response),
                 "ERR 004 FILE_OPEN_FAILED SID:%s\n",
                 SID);

        send(client_fd,
             response,
             strlen(response),
             0);

        continue;
    }

    char file_buffer[4096];

    long remaining = filesize;

    int transfer_failed = 0;

    while (remaining > 0)
    {
        size_t chunk_size;

        if (remaining > (long)sizeof(file_buffer))
        {
            chunk_size = sizeof(file_buffer);
        }
        else
        {
            chunk_size = (size_t)remaining;
        }

        ssize_t n = recv_all(client_fd,
                             file_buffer,
                             chunk_size);

        if (n != (ssize_t)chunk_size)
        {
            transfer_failed = 1;
            break;
        }

        fwrite(file_buffer,
               1,
               chunk_size,
               file);

        remaining -= (long)chunk_size;
    }

    fclose(file);

    if (transfer_failed)
    {
        remove(filepath);

        char response[128];

        snprintf(response,
                 sizeof(response),
                 "ERR 005 TRANSFER_FAILED SID:%s\n",
                 SID);

        send(client_fd,
             response,
             strlen(response),
             0);
    }
    else
    {
        char response[512];

        snprintf(response,
                 sizeof(response),
                 "OK FILE_RECEIVED %s SID:%s\n",
                 filename,
                 SID);

        send(client_fd,
             response,
             strlen(response),
             0);
    }

    continue;
}

/* =================================================
 * GET
 * ================================================= */

if (strncmp(buffer, "GET ", 4) == 0)
{
    char filename[256];

    if (sscanf(buffer,
               "GET %255s",
               filename) != 1)
    {
        char response[128];

        snprintf(response,
                 sizeof(response),
                 "ERR 006 INVALID_GET SID:%s\n",
                 SID);

        send(client_fd,
             response,
             strlen(response),
             0);

        continue;
    }

    char filepath[512];

    snprintf(filepath,
             sizeof(filepath),
             "./agentfiles/IT23583146/%s",
             filename);

    FILE *file = fopen(filepath, "rb");

    if (file == NULL)
    {
        char response[128];

        snprintf(response,
                 sizeof(response),
                 "ERR 007 FILE_NOT_FOUND SID:%s\n",
                 SID);

        send(client_fd,
             response,
             strlen(response),
             0);

        continue;
    }

    /* Find file size */
    fseek(file, 0, SEEK_END);

    long filesize = ftell(file);

    fseek(file, 0, SEEK_SET);

    /* Send file header */
    char response[512];

    snprintf(response,
             sizeof(response),
             "OK FILE_SEND %s %ld SID:%s\n",
             filename,
             filesize,
             SID);

    if (send_all(client_fd,
                 response,
                 strlen(response)) < 0)
    {
        fclose(file);
        break;
    }

    /* Send file bytes */
    char file_buffer[4096];

    size_t bytes_read;

    while ((bytes_read = fread(file_buffer,
                               1,
                               sizeof(file_buffer),
                               file)) > 0)
    {
        if (send_all(client_fd,
                     file_buffer,
                     bytes_read) < 0)
        {
            fclose(file);
            break;
        }
    }

    fclose(file);

    continue;
}

/*Unknown command*/
else
{
    char response[] =
        "ERR 002 UNKNOWN_COMMAND SID:6413\n";

    send(client_fd,
         response,
         strlen(response),
         0);
}

/* End of while loop */
}

    close(client_fd);

    return NULL;
}




int main(void)
{
    int server_fd;
   

    struct sockaddr_in server_addr;
    

    

    /* 1. Create TCP socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* 2. Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /* 3. Bind socket to port */
    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* 4. Start listening */
    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Agent listening on port %d...\n", PORT);

    /* 5. Accept multiple Controllers using threads */

while (1)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    /*
     * Allocate memory for this client's socket.
     * The thread will free it after receiving it.
     */
    int *client_socket = malloc(sizeof(int));

    if (client_socket == NULL)
    {
        perror("malloc");
        continue;
    }

    /*
     * Accept a new Controller connection.
     */
    *client_socket = accept(
        server_fd,
        (struct sockaddr *)&client_addr,
        &client_len
    );

    if (*client_socket < 0)
    {
        perror("accept");
        free(client_socket);
        continue;
    }

    /*
     * Create a new thread to handle this Controller.
     */
    pthread_t thread;

    if (pthread_create(
            &thread,
            NULL,
            handle_client,
            client_socket) != 0)
    {
        perror("pthread_create");

        close(*client_socket);
        free(client_socket);

        continue;
    }

    /*
     * We don't need to wait for the thread.
     * It will clean itself up when finished.
     */
    pthread_detach(thread);
}

/*
 * This code is normally unreachable because
 * the server runs continuously.
 */
close(server_fd);

return 0;
}
