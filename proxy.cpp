#include "proxy.h"
#include "connection.h"

std::shared_ptr<Cache> HTTPProxy::cache = std::make_shared<Cache>();

HTTPProxy::HTTPProxy(int p, size_t count) : port(p), connection_count(count) {
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        throw std::runtime_error("socket failed");
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        perror("setsockopt");
        throw std::runtime_error("setsockopt failed");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(server_fd);
        perror("bind");
        throw std::runtime_error("bind failed");
    }

    if (listen(server_fd, connection_count) < 0) {
        close(server_fd);
        throw std::runtime_error("listen failed");
    }

    printf("Proxy server is listening on port %d\n", port);
}

void* HTTPProxy::client_thread_func(void* arg) {
    try {
        int new_socket = *static_cast<int*>(arg);
        std::shared_ptr<ClientConnection> conn_ptr = std::make_shared<ClientConnection>(new_socket, cache);
        conn_ptr->start();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return NULL;
}

void HTTPProxy::run() {
    int new_socket;
    int addrlen = sizeof(address);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "accept failed" << std::endl;
            continue;
        }

        std::cerr << "New connection" << std::endl;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread_func, (void*)&new_socket) != 0) {
            close(new_socket);
            throw std::runtime_error("pthread_create failed");
        }

        if (pthread_detach(tid) != 0) {
            close(new_socket);
            throw std::runtime_error("pthread_detach failed");
        }
    }
}

HTTPProxy::~HTTPProxy() {
    close(server_fd);
}