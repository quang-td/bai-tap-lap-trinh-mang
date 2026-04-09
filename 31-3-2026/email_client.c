#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 12345
#define BUFFER_SIZE 1024

int isNumber(char *str) {
    for (int i = 0; str[i]; i++) {
        if (!isdigit((unsigned char)str[i]) && str[i] != '\n')
            return 0;
    }
    return 1;
}

void trimNewline(char *str) {
    str[strcspn(str, "\r\n")] = 0;
}

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Loi tao socket\n");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Dia chi IP khong hop le\n");
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Khong ket noi duoc server\n");
        return 1;
    }

    printf("Da ket noi server\n");

    int step = 0;

    while (1) {
        int valread = read(sock, buffer, BUFFER_SIZE);

        if (valread == 0) {
            printf("Server da dong ket noi\n");
            break;
        }

        if (valread < 0) {
            printf("Loi nhan du lieu\n");
            break;
        }

        buffer[valread] = '\0';
        printf("%s", buffer);
        

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            printf("Loi nhap du lieu\n");
            break;
        }

        trimNewline(buffer);

        if (strlen(buffer) == 0) {
            printf("Khong duoc de trong!\n");
            continue;
        }

        if (step == 1) {
            if (!isNumber(buffer)) {
                printf("MSSV phai la so!\n");
                continue;
            }

            if (strlen(buffer) < 6) {
                printf("MSSV phai >= 6 so!\n");
                continue;
            }
        }

        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            printf("Loi gui du lieu\n");
            break;
        }

        send(sock, "\n", 1, 0);

        step++;
    }

    close(sock);
    printf("Da dong client\n");
    return 0;
}