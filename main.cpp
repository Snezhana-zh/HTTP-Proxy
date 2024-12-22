#include <stdio.h>
#include <string>
#include "proxy.h"

#define PORT 8080
#define CONNECTION_COUNT 64

int main() {
    try {
        HTTPProxy* srv = new HTTPProxy(PORT, CONNECTION_COUNT);
        srv->run();
    }
    catch (std::exception& e) {
        std::cerr << "Failed: " << e.what() << std::endl;
    }
}

// http://www.kremlin.ru/

// http://example.com