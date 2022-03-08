cmake_minimum_required(VERSION 3.16)
project(tcp_server VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED  True)
set(CMAKE_BUILD_TYPE DEBUG)
include_directories(../base)
include_directories(../TcpServer)

add_library(tcp_server
        ../TcpServer/Acceptor.cpp
        ../TcpServer/EpollPoller.cpp
        ../TcpServer/EventRegister.cpp
        ../TcpServer/IOThreadPool.cpp
        ../TcpServer/Reactor.cpp
        ../TcpServer/TcpConnection.cpp
        ../TcpServer/TcpServer.cpp
        ../TcpServer/TimerContainer.cpp)
target_link_libraries(tcp_server pthread)
add_library(file_operation ../base/file_operation.cpp)
target_link_libraries(tcp_server file_operation)

add_executable(test_IOThreadPool test_IOThreadPool.cpp)
target_link_libraries(test_IOThreadPool tcp_server)

add_executable(test_acceptor test_acceptor.cpp)
target_link_libraries(test_acceptor tcp_server)

add_executable(test_Reactor test_Reactor.cpp)
target_link_libraries(test_Reactor tcp_server)

add_executable(test_TimerContainer test_TimerContainer.cpp)
target_link_libraries(test_TimerContainer tcp_server)

add_executable(EchoServer EchoServer.cpp)
target_link_libraries(EchoServer tcp_server)

add_executable(CharacterGenerator CharacterGenerator.cpp)
target_link_libraries(CharacterGenerator tcp_server)

add_executable(HttpServer HttpServer.cpp)
target_link_libraries(HttpServer tcp_server)