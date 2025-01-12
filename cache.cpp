#include "cache.h"

CacheItem::CacheItem() : createdTime(std::chrono::steady_clock::now()), 
    complete(false), failed(false), offset(0) {

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);

    int err = pthread_mutex_init(&mutex, &attr);
	pthread_mutexattr_destroy(&attr);
    if (err) {
        throw std::runtime_error("pthread_mutex_init");
    }

	err = pthread_cond_init(&cond, NULL);
    if (err) {
        throw std::runtime_error("pthread_cond_init");
    }

    buffer = (buffer_t*)malloc(sizeof(buffer_t));
    if (buffer == NULL) {
        throw std::runtime_error("buffer == NULL");
    }
    buffer->capacity = CACHE_SIZE;
    buffer->data = (char*)calloc(CACHE_SIZE, sizeof(char));
    if (buffer->data == NULL) {
        throw std::runtime_error("buffer->data == NULL");
    }
}

buffer_t* CacheItem::getData() {
    return buffer;
}

bool CacheItem::timeOut(std::chrono::time_point<std::chrono::steady_clock> time) {
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(time - createdTime);

    if (elapsed >= ttl) {
        return true;
    }
    return false;
}

void CacheItem::setComplete() {
    complete = true;
}

void CacheItem::setErred() {
    failed = true;
}

void CacheItem::setNewBuffer(buffer_t* new_) {
    buffer = new_;
}

unsigned int& CacheItem::getOffset() {
    return offset;
}

void CacheItem::cond_wait() {
    pthread_cond_wait(&cond, &mutex);
}

void CacheItem::cond_broadcast() {
    pthread_cond_broadcast(&cond);
}

bool CacheItem::getComplete() {
    return complete;
}

bool CacheItem::getErred() {
    return failed;
}

void CacheItem::lock() {
    pthread_mutex_lock(&mutex);
}

void CacheItem::unlock() {
    pthread_mutex_unlock(&mutex);
}

CacheItem::~CacheItem() {
    int err = pthread_mutex_destroy(&mutex);
    if (err) {
        std::cerr << "pthread_mutex_destroy() failed" << std::endl;
    }

	err = pthread_cond_destroy(&cond);
    if (err) {
        std::cerr << "pthread_cond_destroy() failed" << std::endl;
    }

    free(buffer->data);

    free(buffer);
}

Cache::Cache() {
    pthread_rwlockattr_t attr;
    int err;

    err = pthread_rwlockattr_init(&attr);
    if (err) throw std::runtime_error("pthread_rwlockattr_init() failed");

    err = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
    if (err) throw std::runtime_error("pthread_rwlockattr_setpshared() failed");

    err = pthread_rwlock_init(&lock, &attr);
    if (err) {
        throw std::runtime_error("pthread_rwlock_init() failed");
    }
}

void Cache::clean() {
    for (auto it = cache_map.begin(); it != cache_map.end();) {
        if (it->second->timeOut(std::chrono::steady_clock::now()) || it->second->getErred()) {
            it = cache_map.erase(it);
        }
        else {
            it++;
        }
    }

    if (cache_map.size() > MAP_SIZE) {
        auto it = cache_map.begin();
        cache_map.erase(it);
    }
}

std::shared_ptr<CacheItem> Cache::putEmpty(std::string key) {
    pthread_rwlock_wrlock(&lock);

    clean();

    auto it = cache_map.find(key);

    if (it == cache_map.end()) {
        auto item = std::make_shared<CacheItem>();
        cache_map[key] = item;
    }

    pthread_rwlock_unlock(&lock);

    return cache_map[key];
}

std::shared_ptr<CacheItem> Cache::get(std::string key) {
    pthread_rwlock_rdlock(&lock);

    auto it = cache_map.find(key);

    std::shared_ptr<CacheItem> data = nullptr;

    if (it != cache_map.end()) {
        data = it->second;
        if (data->getErred()) {
            pthread_rwlock_unlock(&lock);
            return nullptr;
        }
    }
    pthread_rwlock_unlock(&lock);
    return data;
}

std::pair<std::shared_ptr<CacheItem>, bool> Cache::hasElem(std::string key) {
    std::shared_ptr<CacheItem> data = get(key);

    bool found = false;

    if (data != nullptr) found = true;

    return std::pair<std::shared_ptr<CacheItem>, bool>(data, found);
}

Cache::~Cache() {
    pthread_rwlock_wrlock(&lock);
    
    cache_map.clear();

    pthread_rwlock_unlock(&lock);

    int err = pthread_rwlock_destroy(&lock);

    if (err) {
        std::cerr << "pthread_rwlock_destroy() failed: " << std::endl;
    }
}