cmake_minimum_required(VERSION 4.1)
project(ProyectoFinal CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(src third_party)

#Ejecutable 1
add_executable(ProyectoFinal src/main.cpp
        src/benchmark_main.cpp)

#Ejecutable 2
add_executable(server src/server_main.cpp)

#Ejecutable 3
add_executable(benchmark src/benchmark_main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)

if(WIN32)
    target_link_libraries(server PRIVATE ws2_32)
endif()