#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ctype.h>

#define PORT 12345
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

typedef struct {
    int step;
    char name[100];
    char mssv[50];
} ClientState;

// lowercase
void toLowerStr(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

// bỏ ký tự UTF-8 (giữ ASCII)
void removeVietnamese(char *str) {
    char result[200];
    int j = 0;

    for (int i = 0; str[i]; i++) {
        unsigned char c = str[i];
        if (c < 128) { // chỉ giữ ASCII
            result[j++] = c;
        }
    }

    result[j] = '\0';
    strcpy(str, result);
}

// tạo email
void generateEmail(char *name, char *mssv, char *email) {
    char temp[200];
    strcpy(temp, name);

    removeVietnamese(temp);   // ⭐ FIX lỗi tiếng Việt

    char *words[10];
    int count = 0;

    char *token = strtok(temp, " ");
    while (token != NULL && count < 10) {
        words[count++] = token;
        token = strtok(NULL, " ");
    }

    if (count == 0) {
        strcpy(email, "invalid@sis.hust.edu.vn");
        return;
    }

    // tên (cuối)
    char ten[50];
    strcpy(ten, words[count - 1]);
    toLowerStr(ten);

    // chữ cái đầu họ + đệm
    char initials[10] = "";
    for (int i = 0; i < count - 1; i++) {
        char c = tolower((unsigned char)words[i][0]);
        int len = strlen(initials);
        initials[len] = c;
        initials[len + 1] = '\0';
    }

    // 6 số cuối MSSV
    char last6[7];
    int len_mssv = strlen(mssv);

    if (len_mssv >= 6) {
        strncpy(last6, mssv + len_mssv - 6, 6);
        last6[6] = '\0';
    } else {
        strcpy(last6, mssv);
    }

    sprintf(email, "%s.%s%s@sis.hust.edu.vn", ten, initials, last6);
}

int main() {
    int server_fd, new_socket, client_socket[MAX_CLIENTS];
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    fd_set readfds;
    char buffer[BUFFER_SIZE];

    ClientState states[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Server dang chay...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        int max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_socket[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            printf("New connection: %d\n", new_socket);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    states[i].step = 0;
                    send(new_socket, "Nhap ho ten: ", 14, 0);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_socket[i];

            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUFFER_SIZE);

                if (valread <= 0) {
                    close(sd);
                    client_socket[i] = 0;
                    printf("Client disconnected\n");
                } else {
                    buffer[valread] = '\0';
                    buffer[strcspn(buffer, "\r\n")] = 0;

                    if (states[i].step == 0) {
                        strcpy(states[i].name, buffer);
                        states[i].step = 1;
                        send(sd, "Nhap MSSV: ", 12, 0);
                    } else {
                        strcpy(states[i].mssv, buffer);

                        char email[200];
                        generateEmail(states[i].name, states[i].mssv, email);

                        strcat(email, "\n");
                        send(sd, email, strlen(email), 0);

                        close(sd);
                        client_socket[i] = 0;
                    }
                }
            }
        }
    }

    return 0;
}