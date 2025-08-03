cmake_minimum_required(VERSION 3.10)
project(ServerProject)

set(CMAKE_CXX_STANDARD 17)
include_directories(include)

add_executable(Client
    src/client/client_main.cpp
    src/client/client.cpp
)

add_executable(Server
    src/server/server_main.cpp
    src/server/server.cpp
)