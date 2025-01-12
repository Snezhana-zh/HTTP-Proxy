#include "connection.h"

ServerConnection::ServerConnection(char* url_, std::shared_ptr<CacheItem> c, int sock) : cache_item(c), url(url_), client_socket(sock) {
    struct addrinfo hints;
    struct addrinfo *result;

    char service[] = "http";
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int s = getaddrinfo(http_host_from_url(url), service, &hints, &result);

    if (result == NULL) {
        throw std::runtime_error("result == NULL");
    }
    if (s != 0) {
        throw std::runtime_error(strerror(errno));
    }
    server_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    std::cerr << "server connection on sock_fd: " << server_socket << "  for url: "<< url << " host = " << http_host_from_url(url) << std::endl;

    int res = connect(server_socket, result->ai_addr, result->ai_addrlen);
    if (res) {
        close(server_socket);
    }
    freeaddrinfo(result);
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
    int cache_buffer_overflow = 0;

    http_response_t response;
    http_response_init(&response);

    char* tranfer_buffer = (char*)calloc(CACHE_SIZE, sizeof(char));

    while (!response.done) {
        cache_item->lock();

        buffer_t* buffer = cache_item->getData();
        int buffer_len = cache_item->getOffset();

        cache_item->unlock();

        int ret = 0;

        int n = read(server_socket, tranfer_buffer, CACHE_SIZE);

        if (n > 0) {
            err = http_response_parse(&response, tranfer_buffer, n);

            if (err) {
                std::cerr << "error parsing response" << std::endl;
            }
            else {
                int sent = 0;
                while (sent < n) {
                    int n_write = write(client_socket, tranfer_buffer + sent, n - sent);
                    if (n_write < 0) {
                        std::cerr << "ERROR bytes write = " << n_write << strerror(errno) << std::endl;
                        ret = 1;
                        break;
                    }
                    sent += n_write;
                }

                if (buffer_len + n <= CACHE_SIZE) {
                    memcpy(buffer->data + buffer_len, tranfer_buffer, n);
                }
                else {
                    std::cerr << "buffer_len + n = " << buffer_len + n <<  " CACHE_SIZE = " << CACHE_SIZE << std::endl;
                    cache_buffer_overflow = -1;
                }
            }
        }

        cache_item->lock();

        if (n < 0 || err == -1) {
            std::cerr << "ERROR bytes read = " << n << "err = " << err << std::endl;
            cache_item->setErred();
            ret = 1;
        }

        if (cache_buffer_overflow == -1) {
            cache_item->setErred();
        }

        cache_item->getOffset() += n;

        if (response.done) {
            cache_item->setComplete();
        }

        cache_item->cond_broadcast();
        cache_item->unlock();

        if (ret != 0) {
            free(tranfer_buffer);
            return;
        }
    }
    free(tranfer_buffer);
}