#include <exception>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <sstream>
#include <memory>

#include "cache.h"
#include "http.h"

#define REQUEST_BUFSIZE 1024

#define RESPONSE_BUFSIZE (128 * 1024)

class Connection {
public:
    virtual void start() = 0;

    virtual ~Connection() {};

    static void* thread_func(void* arg) {
        try {
            Connection* conn = static_cast<Connection*>(arg);
            conn->start();
            delete conn;
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
        return NULL;
    }
};

class ClientConnection : public Connection {
public:
    ClientConnection(int fd, std::shared_ptr<Cache> cache);

    void start();

    ~ClientConnection();
private:
    int client_socket;
    std::shared_ptr<Cache> cache;
};

class ServerConnection : public Connection {
public:
    ServerConnection(char* url_, std::shared_ptr<CacheItem> cache_item);

    void start();

    int fill_request(char* recv_buffer, size_t recv_count);

    ~ServerConnection();
private:
    char* url;

    std::shared_ptr<CacheItem> cache_item;
    
    int server_socket;
};