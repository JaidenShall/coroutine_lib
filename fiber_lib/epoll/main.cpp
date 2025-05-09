#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10
#define PORT 8888

int main() {
    int listen_fd, conn_fd, epoll_fd, event_count;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    struct epoll_event events[MAX_EVENTS], event;

    // リッスン用のソケットを作成する
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    int yes = 1;
    // 「アドレスはすでに使用中です」エラーを解決する
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // サーバのアドレスとポートを設定する
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // リッスンソケットをサーバアドレスとポートにバインドする
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return -1;
    }

    // 接続をリッスンする
    if (listen(listen_fd, 1024) == -1) {
        perror("listen");
        return -1;
    }

    // epollインスタンスを作成する
    if ((epoll_fd = epoll_create1(0)) == -1) {
        perror("epoll_create1");
        return -1;
    }

    // リッスンソケットをepollインスタンスに追加する
    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
        perror("epoll_ctl");
        return -1;
    }

    while (1) {
        // イベントの発生を待つ
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (event_count == -1) {
            perror("epoll_wait");
            return -1;
        }

        // イベントを処理する
        for (int i = 0; i < event_count; i++) {
            if (events[i].data.fd == listen_fd) {
                // 新しい接続が到着した
                conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
                if (conn_fd == -1) {
                    perror("accept");
                    continue;
                }

                // 新しい接続ソケットをepollインスタンスに追加する
                event.events = EPOLLIN;
                event.data.fd = conn_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &event) == -1) {
                    perror("epoll_ctl");
                    return -1;
                }
            } else {
                // 読み取り可能なデータがある
                char buf[1024];
                int len = read(events[i].data.fd, buf, sizeof(buf) - 1);
                if (len <= 0) {
                    // エラーが発生または接続が切断された場合、接続を閉じる
                    close(events[i].data.fd);
                } else {
                    // HTTPレスポンスを送信する
                    const char *response = "HTTP/1.1 200 OK\r\n"
                                           "Content-Type: text/plain\r\n"
                                           "Content-Length: 1\r\n"
                                           "Connection: keep-alive\r\n"
                                           "\r\n"
                                           "1";
                    write(events[i].data.fd, response, strlen(response));
                    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,events[i].data.fd,NULL);//70007エラーが発生した場合は再度有効にするか、-rコマンドを試す
                    // 接続を閉じる
                    close(events[i].data.fd);
                }
            }
        }
    }

    // リッスンソケットとepollインスタンスを閉じる
    close(listen_fd);
    close(epoll_fd);
    return 0;
}
