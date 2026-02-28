#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

// 必备辅助函数：设置非阻塞
void setNonBlocking(int fd) {
    int opts = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, opts | O_NONBLOCK);
}

int main() {
    // 1. 基础设施
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 16);

    // 2. 注册中心
    int epfd = epoll_create(1);
    struct epoll_event ev, evs[1024];
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    // 3. 动力引擎
    while (1) {
        int n = epoll_wait(epfd, evs, 1024, -1);
        for (int i = 0; i < n; ++i) {
            int cur_fd = evs[i].data.fd;

            // 分支A：新连接处理
            if (cur_fd == listen_fd) {
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);
                int conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &len);
                if (conn_fd < 0) continue;
                
                setNonBlocking(conn_fd); // 极其重要！
                
                struct epoll_event client_ev;
                client_ev.events = EPOLLIN; // 面试默认写 LT 模式最安全
                client_ev.data.fd = conn_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &client_ev);
            } 
            // 分支B：数据读写处理
            else if (evs[i].events & EPOLLIN) {
                char buf[1024];
                int n_read = read(cur_fd, buf, sizeof(buf) - 1);
                
                if (n_read > 0) {
                    write(cur_fd, buf, n_read); // 简单的 Echo 业务
                } else if (n_read == 0) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, cur_fd, nullptr);
                    close(cur_fd);
                }
            }
        }
    }
    return 0;
}