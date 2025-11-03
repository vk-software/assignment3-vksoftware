#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdbool.h>

/*

Based on https://beej.us/guide/bgnet/html/#a-simple-stream-server

*/

#define PORT                9000         // the port users will be connecting to
#define BACKLOG             10          // how many pending connections queue will hold
#define MAXDATASIZE         1000    // max number of bytes we can get at once

#define BUFFER_READ_SIZE    1024
#define WRITE_FILE      "/var/tmp/aesdsocketdata"

// Global variables as they're used in signal handlers

int SOCKET_FD, CLIENT_FD, FILE_FD;

int clean_up () {
    if (SOCKET_FD)
        close(SOCKET_FD);
    if (CLIENT_FD)
        close(CLIENT_FD);
    if (FILE_FD)
        close(FILE_FD);

    remove(WRITE_FILE);
    closelog();
    return 0;
}

void signal_handler(int signal_number){

	if(signal_number == SIGINT || signal_number == SIGTERM){
		syslog(LOG_INFO, "Caught signal, exiting\n");
        clean_up();
		exit(1);
	}
}

int send_file_content(int file_fd, int client_socket_fd) {
    ssize_t bytes_read, bytes_sent, total_sent;
    char buffer[BUFFER_READ_SIZE] = {0};
    
    // Navigate to the beginning of the file
    off_t last_position = lseek(file_fd, 0, SEEK_CUR);
    lseek(file_fd, 0, SEEK_SET);
    printf("Sending file contents back to the client\n");
    
    while ((bytes_read = read(file_fd, buffer, BUFFER_READ_SIZE)) > 0) {
        total_sent = 0;
        printf("Read bytes: %ld, Sending portion: %s\n", bytes_read, buffer);
        while (total_sent < bytes_read) {
            bytes_sent = send(client_socket_fd, buffer + total_sent, bytes_read - total_sent, 0);
            if (bytes_sent < 0) {
                if (errno == EINTR)
                    continue; // retry on interrupt
                perror("");
                syslog(LOG_INFO, "Error: Failed to send data\n");
                lseek(file_fd, last_position, SEEK_SET);
                return -1;
            }
            total_sent += bytes_sent;
        }
    }
    
    if (bytes_read == -1) {
        syslog(LOG_ERR, "Error: Failed to read file '%s': %s\n", WRITE_FILE, strerror(errno));
        lseek(file_fd, last_position, SEEK_SET);
        return -1;
    }
    
    lseek(FILE_FD, last_position, SEEK_SET);
    return 0;

}

int main(int argc, char *argv[]) {
    
    bool daemon = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            daemon = true;
            break;
        }
    }
    
    struct sockaddr_in server_addr, client_addr;
    char buffer[MAXDATASIZE];
    socklen_t client_len = sizeof(client_addr);
    
    openlog(NULL, LOG_PERROR|LOG_PID, LOG_USER);
    
    signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
    
    // TODO: move to content handling?
    FILE_FD = open(WRITE_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);
    // FILE_FD = open(WRITE_FILE, O_CREAT | O_WRONLY | O_APPEND, 0644);
    
    if (!FILE_FD) {
        syslog(LOG_ERR, "Error: cannot open the file '%s': = %s\n", WRITE_FILE, strerror(errno));
        return -1;
    }
    
    SOCKET_FD = socket(AF_INET, SOCK_STREAM, 0);
    if (SOCKET_FD == -1) {
        syslog(LOG_ERR, "Error: Failed to create socket\n");
        return -1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Enables socket reuse
    int opt = 1;
    setsockopt(SOCKET_FD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(SOCKET_FD, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "Error: Socket bind failure\n");
        close(SOCKET_FD);
        return -1;
    }
    
    if (daemon) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "Error: Failed to fork\n");
            return -1;
        }
        
        if (pid > 0) {
            // Parent exit
            exit(0);
        }
        
        // Redirect output
        if (setsid() < 0) {
            perror("setsid");
            syslog(LOG_ERR, "Error: Failed to setsid\n");
        }
        if (chdir("/") < 0) {
            syslog(LOG_ERR, "Error: Failed to change directory\n");
        }
        int fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            if (fd > 2) close(fd);
        }
    }
    
    if (listen(SOCKET_FD, 5) == -1) {
        syslog(LOG_ERR, "Error: Failed to start listening\n");
        close(SOCKET_FD);
        return -1;
    }
    
    while (1) {
        CLIENT_FD = accept(SOCKET_FD, (struct sockaddr *)&client_addr, &client_len);
        if (CLIENT_FD == -1) {
            syslog(LOG_ERR, "Error: Accept failed\n");
            return -1;
        }

        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

        ssize_t bytes_received;
        while ((bytes_received = recv(CLIENT_FD, buffer, sizeof(buffer), 0)) > 0) {
            write(FILE_FD, buffer, bytes_received);
            if (buffer[bytes_received - 1] == '\n') {
                break;
            }
        }

        if (send_file_content(FILE_FD, CLIENT_FD) != 0) {
            syslog(LOG_ERR, "Error: Failed to send data\n");
            return -1;
        }

        close(CLIENT_FD);
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
    }
    
    close(FILE_FD);
    closelog();
    
    return 0;
}