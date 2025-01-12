#include "connection.h"

ClientConnection::ClientConnection(int fd, std::shared_ptr<Cache> c) : client_socket(fd), cache(c) {
    
}

ClientConnection::~ClientConnection() {
    close(client_socket);
}

int write_data_cache(int client, std::shared_ptr<CacheItem> cache_item) {
    int sent = 0;

    while (1) {
        cache_item->lock();

        while (sent == cache_item->getOffset() && !cache_item->getComplete() && !cache_item->getErred()) {
            cache_item->cond_wait();
        }

        if (cache_item->getErred()) {
            cache_item->unlock();
            std::cerr << "ERROR write bytes count getErred() " << std::endl;
            return 1;
        }

        if (cache_item->getComplete() && cache_item->getOffset() == sent) {
            cache_item->unlock();
            break;
        }

        int len = cache_item->getOffset();
        buffer_t* buffer = cache_item->getData();

        cache_item->unlock();

        while (sent < len) {
            int n = write(client, buffer->data + sent, len - sent);

            if (n < 0) {
                std::cerr << "ERROR write bytes count n = " << n << strerror(errno) << std::endl;
                return 1;
            }

            sent += n;
        }
    }
    
    return 0;
}

int read_req_from_client(http_request_t* request, char** res, int& len, int client) {
    int size = REQUEST_BUFSIZE;
    char *request_buf = (char*)calloc(size, 1);

    int recv_count = 0;

    while (!request->done) {
        int n = read(client, request_buf + recv_count, size - recv_count);
        if (n < 0) {
            std::cerr << "ERROR read n = " << n << std::endl;
            free(request_buf);
            return 1;
        }

        int err = http_request_parse(request, request_buf + recv_count, n);
        if (err) {
            free(request_buf);
            return 1;
        }

        recv_count += n;
    }

    *res = request_buf;
    len = recv_count;
    return 0;
}

void ClientConnection::start() {
    int err;
    char *request_buf;
    int recv_count;
    http_request_t request;
    http_request_init(&request);

    err = read_req_from_client(&request, &request_buf, recv_count, client_socket);
    if (err != 0) {
        std::cerr << "read_req_from_client" << std::endl;
        return;
    }

    auto elem = cache->hasElem(request.url);

    if (elem.second) {
        // Если данные есть в кэше, отправляем их клиенту
        free(request_buf);
        err = write_data_cache(client_socket, elem.first);
        if (err) {
            std::cerr << "write_data_cache" << std::endl;
            return;
        }
        std::cerr << "DATA CACHE url = " << request.url << std::endl;
    } else {
        // Если данных нет в кэше, создаем новый поток для загрузки данных
        try {
        std::cerr << "DATA START url = " << request.url << std::endl;
        auto cache_it = cache->putEmpty(request.url);

        ServerConnection* srv_conn = new ServerConnection(request.url, cache_it, client_socket);
        err = srv_conn->fill_request(request_buf, recv_count);
        if (err != 0) {
            std::cerr << "fill_request" << std::endl;
            free(request_buf);
            delete srv_conn;
            return;
        }

        free(request_buf);

        pthread_t srv_thread;
        if (pthread_create(&srv_thread, NULL, Connection::thread_func, (void*)srv_conn) != 0) {
            throw std::runtime_error("pthread_create failed");
        }

        pthread_join(srv_thread, NULL);

        std::cerr << "DATA END" << std::endl;

        }
        catch(std::exception& ex) {
            std::cerr << "ERROR: " << ex.what() << std::endl;
        }
    }
}