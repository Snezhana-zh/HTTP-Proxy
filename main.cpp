#include <stdio.h>
#include <string>
#include <signal.h>
#include "proxy.h"

#define PORT 8080
#define CONNECTION_COUNT 64

void signal_callback_handler(int signum){
}

int main() {
    signal(SIGPIPE, signal_callback_handler);
    
    try {
        HTTPProxy* srv = new HTTPProxy(PORT, CONNECTION_COUNT);
        srv->run();
    }
    catch (std::exception& e) {
        std::cerr << "Failed: " << e.what() << std::endl;
    }
}