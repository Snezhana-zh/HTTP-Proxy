#include "connection.h"

ServerConnection::ServerConnection(char* url_, std::shared_ptr<CacheItem> c) : cache_item(c), url(url_) {
    struct addrinfo hints;
    struct addrinfo *result;

    char service[] = "http";
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;   
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;      
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    std::cerr << "server connection on sock_fd: " << server_socket << "  for url: "<< url << " host = " << http_host_from_url(url) << std::endl;

    int s = getaddrinfo(http_host_from_url(url), service, &hints, &result);
    if (s != 0) {
        throw std::runtime_error(strerror(errno));
    }

    int res = connect(server_socket, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    if (res) {
        close(server_socket);
    }
}

ServerConnection::~ServerConnection() {
    close(server_socket);
}

int ServerConnection::fill_request(char* recv_buffer, size_t recv_count) {
    int sent = 0;
    while (sent < recv_count) {
        int n = write(server_socket, recv_buffer + sent, recv_count - sent);

        if (n <= 0) {
            return 1;
        }

        sent += n;
    }

    return 0;
}

void ServerConnection::start() {
    int err;

    http_response_t response;
    http_response_init(&response);

    while (!response.done) {
        cache_item->lock();

        if (cache_item->getData()->capacity <= cache_item->getOffset()) {

            buffer_t *old = cache_item->getData();

            buffer_t *new_ = (buffer_t*)malloc(sizeof(buffer_t));
            new_->capacity = old->capacity * 2;
            new_->data = (char*)malloc(old->capacity * 2);

            memcpy(new_->data, old->data, cache_item->getOffset());

            cache_item->setNewBuffer(new_);

            free(old->data);
            free(old);
        }

        buffer_t* buffer = cache_item->getData();
        int buffer_len = cache_item->getOffset();

        cache_item->unlock();

        int n = read(
            server_socket,
            buffer->data + buffer_len,
            buffer->capacity - buffer_len
        );

        if (n > 0) {
            err = http_response_parse(
                &response, buffer->data + buffer_len, n
            );

            if (err) {
                std::cerr << "error parsing response" << std::endl;
            }
        }

        cache_item->lock();

        int ret = 0;

        if (n <= 0 || err == -1) {
            cache_item->setErred();
            ret = 1;
        }

        cache_item->getOffset() += n;

        if (response.done) {
            cache_item->setComplete();
        }

        cache_item->cond_broadcast();
        cache_item->unlock();

        if (ret != 0) {
            return;
        }
    }
}