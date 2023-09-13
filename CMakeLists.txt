cmake_minimum_required(VERSION 3.10)
project(jirafeau)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(CURL REQUIRED)

include_directories(include)

add_executable(jirafeau src/main.c src/jirafeau.c)

target_link_libraries(jirafeau ${CURL_LIBRARIES})
