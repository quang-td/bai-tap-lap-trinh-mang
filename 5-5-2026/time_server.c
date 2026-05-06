#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 9000
#define BUF_SIZE 256

int valid_format(char *fmt) {
    return strcmp(fmt, "dd/mm/yyyy") == 0 ||
           strcmp(fmt, "dd/mm/yy") == 0 ||
           strcmp(fmt, "mm/dd/yyyy") == 0 ||
           strcmp(fmt, "mm/dd/yy") == 0;
}

void format_time(char *fmt, char *out) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    if (strcmp(fmt, "dd/mm/yyyy") == 0) {
        sprintf(out, "%02d/%02d/%04d",
                tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
    } else if (strcmp(fmt, "dd/mm/yy") == 0) {
        sprintf(out, "%02d/%02d/%02d",
                tm->tm_mday, tm->tm_mon + 1, (tm->tm_year + 1900) % 100);
    } else if (strcmp(fmt, "mm/dd/yyyy") == 0) {
        sprintf(out, "%02d/%02d/%04d",
                tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900);
    } else if (strcmp(fmt, "mm/dd/yy") == 0) {
        sprintf(out, "%02d/%02d/%02d",
                tm->tm_mon + 1, tm->tm_mday, (tm->tm_year + 1900) % 100);
    }
}

void handle_client(int client) {
    char buf[BUF_SIZE];
    char cmd[50], fmt[50];

    int n = recv(client, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        close(client);
        exit(0);
    }

    buf[n] = 0;

    if (sscanf(buf, "%s %s", cmd, fmt) != 2) {
        send(client, "Invalid command\n", 16, 0);
        close(client);
        exit(0);
    }

    if (strcmp(cmd, "GET_TIME") != 0) {
        send(client, "Invalid command\n", 16, 0);
        close(client);
        exit(0);
    }

    if (!valid_format(fmt)) {
        send(client, "Invalid format\n", 15, 0);
        close(client);
        exit(0);
    }

    char result[100];
    format_time(fmt, result);

    send(client, result, strlen(result), 0);

    close(client);
    exit(0);
}

int main() {
    int server = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    printf("Time server running on port %d...\n", PORT);

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