# Указываем минимальную версию CMake
cmake_minimum_required(VERSION 3.14)

# Указываем имя проекта
project(MyProxyProject C CXX)

# Указываем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Подключаем FetchContent
include(FetchContent)

# Загружаем llhttp из указанного URL
FetchContent_Declare(
  llhttp
  URL "https://github.com/nodejs/llhttp/archive/refs/tags/release/v8.1.0.tar.gz"
)

# Устанавливаем параметры сборки для llhttp
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")

# Делаем библиотеку llhttp доступной
FetchContent_MakeAvailable(llhttp)

# Добавляем исходные файлы вашего проекта
set(SOURCES
    main.cpp
    client_connection.cpp
    server_connection.cpp
    proxy.cpp
    cache.cpp
    http.cpp
)

include_directories(${llhttp_SOURCE_DIR}/include)

# Добавляем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES})

# Подключаем библиотеку llhttp_static
target_link_libraries(${PROJECT_NAME} llhttp_static)