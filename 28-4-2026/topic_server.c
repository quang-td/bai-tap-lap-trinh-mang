#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 9000
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define MAX_TOPICS 10
#define TOPIC_LEN 50

typedef struct {
    int fd;
    char topics[MAX_TOPICS][TOPIC_LEN];
    int topic_count;
} Client;

Client clients[MAX_CLIENTS];
struct pollfd fds[MAX_CLIENTS];

void send_msg(int fd, const char *msg) {
    send(fd, msg, strlen(msg), 0);
}

int find_client(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd == fd) return i;
    return -1;
}

void add_client(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) {
            clients[i].fd = fd;
            clients[i].topic_count = 0;
            return;
        }
    }
}

void remove_client(int fd) {
    int idx = find_client(fd);
    if (idx != -1) {
        clients[idx].fd = -1;
        clients[idx].topic_count = 0;
    }
}

void subscribe(int fd, char *topic) {
    int idx = find_client(fd);
    if (idx == -1) return;

    Client *c = &clients[idx];

    for (int i = 0; i < c->topic_count; i++)
        if (strcmp(c->topics[i], topic) == 0)
            return;

    if (c->topic_count < MAX_TOPICS) {
        strcpy(c->topics[c->topic_count++], topic);
    }
}

void unsubscribe(int fd, char *topic) {
    int idx = find_client(fd);
    if (idx == -1) return;

    Client *c = &clients[idx];

    for (int i = 0; i < c->topic_count; i++) {
        if (strcmp(c->topics[i], topic) == 0) {
            for (int j = i; j < c->topic_count - 1; j++)
                strcpy(c->topics[j], c->topics[j + 1]);
            c->topic_count--;
            return;
        }
    }
}

int is_subscribed(Client *c, char *topic) {
    for (int i = 0; i < c->topic_count; i++)
        if (strcmp(c->topics[i], topic) == 0)
            return 1;
    return 0;
}

void publish(int sender_fd, char *topic, char *msg) {
    char out[BUFFER_SIZE];
    snprintf(out, sizeof(out), "[%s] %s\n", topic, msg);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 &&
            clients[i].fd != sender_fd &&
            is_subscribed(&clients[i], topic)) {

            send(clients[i].fd, out, strlen(out), 0);
        }
    }
}

int main() {
    int server_fd;
    struct sockaddr_in addr;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        fds[i].fd = -1;
        clients[i].fd = -1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    printf("Server running on port %d...\n", PORT);

    char buffer[BUFFER_SIZE];

    while (1) {
        int activity = poll(fds, MAX_CLIENTS, -1);

        if (activity < 0) {
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_fd, NULL, NULL);

            for (int i = 1; i < MAX_CLIENTS; i++) {
                if (fds[i].fd == -1) {
                    fds[i].fd = client_fd;
                    fds[i].events = POLLIN;
                    break;
                }
            }

            add_client(client_fd);

            send_msg(client_fd,
                "Welcome to PUB/SUB server!\n"
                "Commands:\n"
                "  SUB <topic>\n"
                "  UNSUB <topic>\n"
                "  PUB <topic> <msg>\n"
                "  LIST\n"
                "  HELP\n"
                "  EXIT\n\n"
            );
        }

        for (int i = 1; i < MAX_CLIENTS; i++) {
            if (fds[i].fd == -1) continue;

            if (fds[i].revents & POLLIN) {
                int fd = fds[i].fd;

                int n = recv(fd, buffer, BUFFER_SIZE - 1, 0);

                if (n <= 0) {
                    close(fd);
                    remove_client(fd);
                    fds[i].fd = -1;
                    continue;
                }

                buffer[n] = '\0';
                buffer[strcspn(buffer, "\r\n")] = 0;

                char cmd[10], topic[TOPIC_LEN], msg[BUFFER_SIZE];

                if (sscanf(buffer, "%s", cmd) <= 0) {
                    send_msg(fd, "Invalid command\n");
                    continue;
                }

                if (strcmp(cmd, "SUB") == 0) {
                    if (sscanf(buffer, "%*s %s", topic) != 1) {
                        send_msg(fd, "Usage: SUB <topic>\n");
                    } else {
                        subscribe(fd, topic);
                        send_msg(fd, "Subscribed\n");
                    }
                }

                else if (strcmp(cmd, "UNSUB") == 0) {
                    if (sscanf(buffer, "%*s %s", topic) != 1) {
                        send_msg(fd, "Usage: UNSUB <topic>\n");
                    } else {
                        unsubscribe(fd, topic);
                        send_msg(fd, "Unsubscribed\n");
                    }
                }

                else if (strcmp(cmd, "PUB") == 0) {
                    if (sscanf(buffer, "%*s %s %[^\n]", topic, msg) < 2) {
                        send_msg(fd, "Usage: PUB <topic> <msg>\n");
                    } else {
                        publish(fd, topic, msg);
                        send_msg(fd, "Published\n");
                    }
                }

                else if (strcmp(cmd, "LIST") == 0) {
                    int idx = find_client(fd);
                    if (idx != -1) {
                        Client *c = &clients[idx];
                        if (c->topic_count == 0) {
                            send_msg(fd, "No topics\n");
                        } else {
                            char out[BUFFER_SIZE] = "Your topics:\n";
                            for (int j = 0; j < c->topic_count; j++) {
                                strcat(out, "- ");
                                strcat(out, c->topics[j]);
                                strcat(out, "\n");
                            }
                            send_msg(fd, out);
                        }
                    }
                }

                else if (strcmp(cmd, "HELP") == 0) {
                    send_msg(fd,
                        "Commands:\n"
                        "SUB <topic>\n"
                        "UNSUB <topic>\n"
                        "PUB <topic> <msg>\n"
                        "LIST\n"
                        "EXIT\n");
                }

                else if (strcmp(cmd, "EXIT") == 0) {
                    send_msg(fd, "Bye!\n");
                    close(fd);
                    remove_client(fd);
                    fds[i].fd = -1;
                }

                else {
                    send_msg(fd, "Unknown command. Type HELP\n");
                }
            }
        }
    }

    close(server_fd);
    return 0;
}