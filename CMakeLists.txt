cmake_minimum_required(VERSION 3.17)
project(socket_echo C)

set(CMAKE_C_STANDARD 90)

add_executable(server src/server/server.c)
add_executable(client src/client/client.c)
