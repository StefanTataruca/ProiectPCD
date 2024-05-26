#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pthread.h>
#include "threadpool.h"
#include <cjson/cJSON.h>
#include <limits.h>

#define MAX_BUFFER_LENGTH 1024
#define PORT 8080
#define BUFFER_SIZE 1024
#define THREAD_COUNT 5
#define QUEUE_SIZE 10

pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;
int connection_count = 0;


void trim_newline(char *str) {
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL) *pos = '\0';
    if ((pos = strchr(str, '\r')) != NULL) *pos = '\0';
}


void extract_metadata(const char *filename, int client_socket) {
    char buffer[BUFFER_SIZE];
    FILE *file = fopen(filename, "r");
    if (file) {
        while (fgets(buffer, sizeof(buffer), file)) {
            send(client_socket, buffer, strlen(buffer), 0);
        }
        fclose(file);
    } else {
        send(client_socket, "Failed to open metadata file.\n", strlen("Failed to open metadata file.\n"), 0);
        perror("Failed to open metadata file");
    }
}

int authenticate_client(const char *username, const char *password) {
    if ((strcmp(username, "admin") == 0 && strcmp(password, "adminpass") == 0) ||
        (strcmp(username, "simple") == 0 && strcmp(password, "simplepass") == 0) ||
        (strcmp(username, "remote") == 0 && strcmp(password, "remotepass") == 0)) {
        return 1;
    }
    return 0;
}

const char* get_role(const char *username) {
    if (strcmp(username, "admin") == 0) {
        return "admin";
    } else if (strcmp(username, "simple") == 0) {
        return "simple";
    } else if (strcmp(username, "remote") == 0) {
        return "remote";
    }
    return "unknown";
}


void log_activity(const char *message) {
    FILE *log_file = fopen("server.log", "a");
    if (log_file == NULL) {
        perror("Could not open server log file");
        return;
    }
    time_t now = time(NULL);
    fprintf(log_file, "[%s] %s\n", ctime(&now), message);
    fclose(log_file);
}


void log_change(const char *filename, const char *operation, const char *message) {
    FILE *log_file = fopen("changes.log", "a");
    if (log_file == NULL) {
        perror("Could not open changes log file");
        return;
    }
    time_t now = time(NULL);
    fprintf(log_file, "[%s] [File: %s] [%s] %s\n", ctime(&now), filename, operation, message);
    fclose(log_file);
}

void update_connection_count(int delta) {
    pthread_mutex_lock(&connection_mutex);
    connection_count += delta;
    printf("Current connection count: %d\n", connection_count);
    pthread_mutex_unlock(&connection_mutex);
}

void monitor_server() {
    while (1) {
        printf("Monitoring server...\n");
        sleep(10);
    }
}

void upload_metadata(const char *filename, const char *metadata) {
    char abs_path[PATH_MAX];
    realpath(filename, abs_path);
    FILE *file = fopen(abs_path, "w");
    if (file) {
        fprintf(file, "%s", metadata);
        fclose(file);
        log_change(abs_path, "upload", "User uploaded a new metadata file");
    } else {
        perror("Failed to upload metadata");
    }
}

int file_exists(const char *filename) {
    char abs_path[PATH_MAX];
    realpath(filename, abs_path);
    FILE *file = fopen(abs_path, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}


void edit_metadata(const char *filename, const char *new_metadata) {
    char abs_path[PATH_MAX];
    realpath(filename, abs_path);
    FILE *file = fopen(abs_path, "w");
    if (file) {
        fprintf(file, "%s", new_metadata);
        fclose(file);
        log_change(abs_path, "edit", "Admin edited the metadata file");
    } else {
        perror("Failed to edit metadata");
    }
}


void client_handler(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while (1) {

        send(client_socket, "Username: ", strlen("Username: "), 0);
        bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) return;
        buffer[bytes_read] = '\0';
        trim_newline(buffer);
        char username[50];
        strncpy(username, buffer, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';

        send(client_socket, "Password: ", strlen("Password: "), 0);
        bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) return;
        buffer[bytes_read] = '\0';
        trim_newline(buffer);
        char password[50];
        strncpy(password, buffer, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';

        if (!authenticate_client(username, password)) {
            send(client_socket, "Authentication failed. Please try again.\n", strlen("Authentication failed. Please try again.\n"), 0);
            continue;
        }

        const char *role = get_role(username);
        char response[BUFFER_SIZE];

        if (strcmp(role, "admin") == 0) {
            snprintf(response, sizeof(response), "Hello Admin! You have full access. Type 'list' to list all files, 'view <filename>' to view a file, 'edit <filename>' to edit a file, or 'exit' to disconnect.\n");
            send(client_socket, response, strlen(response), 0);
            log_activity("Admin user authenticated");

            while (1) {
                memset(buffer, 0, sizeof(buffer));
                bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
                if (bytes_read <= 0) break;
                buffer[bytes_read] = '\0';
                trim_newline(buffer);

                if (strcmp(buffer, "list") == 0) {
                    system("ls > files_list.txt");
                    extract_metadata("files_list.txt", client_socket);
                } else if (strncmp(buffer, "view ", 5) == 0) {
                    char *filename = buffer + 5;
                    extract_metadata(filename, client_socket);
                } else if (strncmp(buffer, "edit ", 5) == 0) {
                    char *filename = buffer + 5;
                    if (!file_exists(filename)) {
                        send(client_socket, "File does not exist. Cannot edit.\n", strlen("File does not exist. Cannot edit.\n"), 0);
                        continue;
                    }
                    send(client_socket, "Send the new metadata content. End with an empty line:\n", strlen("Send the new metadata content. End with an empty line:\n"), 0);
                    char new_metadata[BUFFER_SIZE * 10] = {0};
                    while (1) {
                        memset(buffer, 0, sizeof(buffer));
                        bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
                        if (bytes_read <= 0) break;
                        buffer[bytes_read] = '\0';
                        trim_newline(buffer);
                        if (strlen(buffer) == 0) break;
                        strncat(new_metadata, buffer, sizeof(new_metadata) - strlen(new_metadata) - 1);
                        strncat(new_metadata, "\n", sizeof(new_metadata) - strlen(new_metadata) - 1);
                    }
                    edit_metadata(filename, new_metadata);
                    send(client_socket, "Metadata edited and file updated.\n", strlen("Metadata edited and file updated.\n"), 0);
                } else if (strcmp(buffer, "exit") == 0) {
                    break;
                } else {
                    send(client_socket, "Unknown command\n", strlen("Unknown command\n"), 0);
                }
            }
        } else if (strcmp(role, "simple") == 0) {
            snprintf(response, sizeof(response), "Hello Simple User! You can upload a new metadata file or extract metadata. Type 'upload' to upload a new metadata file, 'extract' to extract metadata, or 'exit' to disconnect.\n");
            send(client_socket, response, strlen(response), 0);
            log_activity("Simple user authenticated");

            while (1) {
                memset(buffer, 0, sizeof(buffer));
                bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
                if (bytes_read <= 0) break;
                buffer[bytes_read] = '\0';
                trim_newline(buffer);

                if (strcmp(buffer, "upload") == 0) {
                    send(client_socket, "Send the metadata content. End with an empty line:\n", strlen("Send the metadata content. End with an empty line:\n"), 0);
                    char metadata[BUFFER_SIZE * 10] = {0};
                    while (1) {
                        memset(buffer, 0, sizeof(buffer));
                        bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
                        if (bytes_read <= 0) break;
                        buffer[bytes_read] = '\0';
                        trim_newline(buffer);
                        if (strlen(buffer) == 0) break;
                        strncat(metadata, buffer, sizeof(metadata) - strlen(metadata) - 1);
                        strncat(metadata, "\n", sizeof(metadata) - strlen(metadata) - 1);
                    }

                    send(client_socket, "Enter the name of the file (without extension):\n", strlen("Enter the name of the file (without extension):\n"), 0);
                    memset(buffer, 0, sizeof(buffer));
                    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
                    if (bytes_read <= 0) break;
                    buffer[bytes_read] = '\0';
                    trim_newline(buffer);


                    if (strlen(buffer) > MAX_BUFFER_LENGTH - 5) {
                        send(client_socket, "Filename too long.\n", strlen("Filename too long.\n"), 0);
                        continue;
                    }

                    char filename[1029];
                    snprintf(filename, sizeof(filename), "%s.json", buffer);

                    if (file_exists(filename)) {
                        send(client_socket, "File already exists. Please choose a different name.\n", strlen("File already exists. Please choose a different name.\n"), 0);
                        continue;
                    }

                    upload_metadata(filename, metadata);
                    send(client_socket, "Metadata uploaded and file created.\n", strlen("Metadata uploaded and file created.\n"), 0);
                } else if (strcmp(buffer, "extract") == 0) {
                    send(client_socket, "Enter the filename to extract metadata from (without extension):\n", strlen("Enter the filename to extract metadata from (without extension):\n"), 0);
                    memset(buffer, 0, sizeof(buffer));
                    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
                    if (bytes_read <= 0) break;
                    buffer[bytes_read] = '\0';
                    trim_newline(buffer);

                    if (strlen(buffer) > MAX_BUFFER_LENGTH - 5) {
                        send(client_socket, "Filename too long.\n", strlen("Filename too long.\n"), 0);
                        continue;
                    }

                    char filename[1029]; 
                    snprintf(filename, sizeof(filename), "%s.json", buffer);

                    extract_metadata(filename, client_socket);
                    send(client_socket, "Metadata extracted.\n", strlen("Metadata extracted.\n"), 0);
                } else if (strcmp(buffer, "exit") == 0) {
                    break;
                } else {
                    send(client_socket, "Unknown command\n", strlen("Unknown command\n"), 0);
                }
            }
        } else if (strcmp(role, "remote") == 0) {
            snprintf(response, sizeof(response), "Hello Remote User! You have remote access. Type 'exit' to disconnect.\n");
            send(client_socket, response, strlen(response), 0);
            log_activity("Remote user authenticated");
        } else {
            snprintf(response, sizeof(response), "Hello! Your role is not recognized.\n");
            send(client_socket, response, strlen(response), 0);
            log_activity("Unknown role authenticated");
        }

        close(client_socket);
        update_connection_count(-1);
        break;
    }
}

void start_server() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    printf("====================================================\n");
    printf("=             Server is Starting Up                =\n");
    printf("====================================================\n");

    threadpool_t *pool = threadpool_create(THREAD_COUNT, QUEUE_SIZE);

    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, (void *)monitor_server, NULL);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0) {
        update_connection_count(1);
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;
        threadpool_add(pool, client_handler, pclient);
    }

    close(server_fd);
    threadpool_destroy(pool, 0);
    pthread_cancel(monitor_thread);
    pthread_join(monitor_thread, NULL);
}
