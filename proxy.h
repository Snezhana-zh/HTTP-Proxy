#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdexcept>
#include <vector>
#include <memory>

#include "cache.h"

class HTTPProxy {
public:
    HTTPProxy(int p, size_t count);

    static void* client_thread_func(void* arg);

    void run();

    ~HTTPProxy();
private:
    int server_fd;
    struct sockaddr_in address;
    int port;
    size_t connection_count;
    static std::shared_ptr<Cache> cache;
};