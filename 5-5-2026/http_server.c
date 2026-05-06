#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

#define PORT 9000
#define WORKERS 5
#define BUF_SIZE 256

void handle_client(int client)
{
    char buf[BUF_SIZE];

    int ret = recv(client, buf, sizeof(buf) - 1, 0);
    if (ret <= 0)
        return;

    buf[ret] = 0;
    puts(buf);

    char *msg =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body><h1>Xin chao cac ban</h1></body></html>";

    send(client, msg, strlen(msg), 0);
}

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);

    printf("Server running on port %d...\n", PORT);

    for (int i = 0; i < WORKERS; i++)
    {
        if (fork() == 0)
        {
            // Process con
            while (1)
            {
                int client = accept(listener, NULL, NULL);
                printf("[Worker %d] Client: %d\n", getpid(), client);

                handle_client(client);

                close(client);
            }
            exit(0);
        }
    }

    while (1)
        pause();

    return 0;
}