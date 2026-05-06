#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 9000
#define BUF_SIZE 1024

int check_login(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;

    char u[100], p[100];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

void handle_client(int client) {
    char buf[BUF_SIZE];

    send(client, "Username: ", 10, 0);
    recv(client, buf, BUF_SIZE, 0);
    buf[strcspn(buf, "\r\n")] = 0;
    char user[100];
    strcpy(user, buf);

    send(client, "Password: ", 10, 0);
    recv(client, buf, BUF_SIZE, 0);
    buf[strcspn(buf, "\r\n")] = 0;
    char pass[100];
    strcpy(pass, buf);

    if (!check_login(user, pass)) {
        send(client, "Login failed!\n", 14, 0);
        close(client);
        exit(0);
    }

    send(client, "Login success!\n", 15, 0);

    while (1) {
        send(client, "cmd> ", 5, 0);

        int n = recv(client, buf, BUF_SIZE, 0);
        if (n <= 0) break;

        buf[n] = 0;
        buf[strcspn(buf, "\r\n")] = 0;

        if (strcmp(buf, "exit") == 0) break;

        char command[BUF_SIZE];
        snprintf(command, sizeof(command), "%s > out.txt", buf);
        system(command);

        FILE *f = fopen("out.txt", "r");
        if (!f) {
            send(client, "Error reading output\n", 21, 0);
            continue;
        }

        while (fgets(buf, BUF_SIZE, f)) {
            send(client, buf, strlen(buf), 0);
        }

        fclose(f);
    }

    close(client);
    exit(0);
}

int main() {
    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    printf("Server running on port %d...\n", PORT);

    while (1) {
        int client = accept(server, NULL, NULL);

        if (fork() == 0) {
            close(server);
            handle_client(client);
        }

        close(client);
    }

    return 0;
}