#ifndef CACHE_H
#define CACHE_H

#include <map>
#include <iostream>
#include <string.h>
#include <pthread.h>
#include <exception>
#include <errno.h>
#include <chrono>
#include <memory>

#define MAP_SIZE 128

#define CACHE_SIZE 1024

struct buffer_t {
    char* data;
    size_t capacity;
};

class CacheItem {
private:
    std::chrono::time_point<std::chrono::steady_clock> createdTime;
    std::chrono::minutes ttl{1};
    buffer_t* buffer;
    bool complete;
    bool failed;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    unsigned int offset; // available count to sent client
public:
    CacheItem();
    
    buffer_t* getData();
    void setNewBuffer(buffer_t* new_);

    bool timeOut(std::chrono::time_point<std::chrono::steady_clock> time);

    void setComplete();
    void setErred();

    bool getComplete();
    bool getErred();

    unsigned int& getOffset();

    void cond_wait();
    void cond_broadcast();
    void lock();
    void unlock();

    ~CacheItem();
};

class Cache {
public:
    Cache();

    std::shared_ptr<CacheItem> putEmpty(std::string key);

    std::shared_ptr<CacheItem> get(std::string key);

    std::pair<std::shared_ptr<CacheItem>, bool> hasElem(std::string key);

    void clean();

    ~Cache();
private:
    std::map<std::string, std::shared_ptr<CacheItem>> cache_map;

    pthread_rwlock_t lock;
};

#endif