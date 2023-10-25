cmake_minimum_required(VERSION 3.22)
project(cooper)

option(COOPER_TEST "Build cooper test" ON)

set(CMAKE_CXX_STANDARD 17)
set(ENABLE_MYSQL ON)
set(BUILD_UNIT_TESTS OFF)
set(BUILD_EXAMPLES OFF)

add_subdirectory(third_party/json)
add_subdirectory(third_party/ormpp)

add_library(cooper
        #net
        cooper/net/CallBacks.hpp
        cooper/net/Poller.cpp
        cooper/net/Channel.hpp
        cooper/net/Channel.cpp
        cooper/net/EventLoop.hpp
        cooper/net/EventLoop.cpp
        cooper/net/Poller.hpp
        cooper/net/EpollPoller.hpp
        cooper/net/EpollPoller.cpp
        cooper/net/Timer.cpp
        cooper/net/Timer.hpp
        cooper/net/TimerQueue.cpp
        cooper/net/TimerQueue.hpp
        cooper/net/EventLoopThread.hpp
        cooper/net/EventLoopThread.cpp
        cooper/net/EventLoopThreadPool.hpp
        cooper/net/EventLoopThreadPool.cpp
        cooper/net/InetAddress.hpp
        cooper/net/InetAddress.cpp
        cooper/net/Socket.hpp
        cooper/net/Socket.cpp
        cooper/net/Acceptor.hpp
        cooper/net/Acceptor.cpp
        cooper/net/Connector.hpp
        cooper/net/Connector.cpp
        cooper/net/TLSPolicy.hpp
        cooper/net/Certificate.hpp
        cooper/net/OpenSSLProvider.cpp
        cooper/net/TcpConnection.hpp
        cooper/net/TLSProvider.hpp
        cooper/net/TcpConnectionImpl.hpp
        cooper/net/TcpConnectionImpl.cpp
        cooper/net/TcpServer.hpp
        cooper/net/TcpServer.cpp
        cooper/net/TcpClient.hpp
        cooper/net/TcpClient.cpp
        cooper/net/AppTcpServer.hpp
        cooper/net/AppTcpServer.cpp
        cooper/net/Http.hpp
        cooper/net/Http.cpp
        cooper/net/HttpServer.hpp
        cooper/net/HttpServer.cpp
        # util
        cooper/util/LogStream.cpp
        cooper/util/LogStream.hpp
        cooper/util/Logger.cpp
        cooper/util/Logger.hpp
        cooper/util/AsyncLogWriter.hpp
        cooper/util/AsyncLogWriter.cpp
        cooper/util/NonCopyable.hpp
        cooper/util/Date.hpp
        cooper/util/Date.cpp
        cooper/util/Funcs.hpp
        cooper/util/LockFreeQueue.hpp
        cooper/util/MsgBuffer.hpp
        cooper/util/MsgBuffer.cpp
        cooper/util/Utilities.hpp
        cooper/util/Utilities.cpp
        cooper/util/TimingWheel.cpp
        cooper/util/TimingWheel.hpp
        cooper/util/Semaphore.hpp
        cooper/util/ThreadPool.cpp
        cooper/util/ThreadPool.hpp)

set_target_properties(cooper PROPERTIES LINKER_LANGUAGE CXX)
find_package(OpenSSL REQUIRED)
target_link_libraries(cooper PUBLIC OpenSSL::SSL)
target_link_libraries(cooper PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(cooper PUBLIC ${MYSQL_LIBRARY})
target_include_directories(cooper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(cooper PUBLIC third_party/ormpp)
target_include_directories(cooper PUBLIC ${MYSQL_INCLUDE_DIR} include)
target_include_directories(cooper PUBLIC third_party/ormpp/include)
target_compile_definitions(cooper PUBLIC ORMPP_ENABLE_MYSQL)

if (COOPER_TEST)
    file(GLOB_RECURSE TEST_SOURCES "test/*.cpp")
    foreach (TEST_SOURCE ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_SOURCE}
                test/ThreadPoolTest.cpp)
        target_link_libraries(${TEST_NAME} PRIVATE cooper)
    endforeach ()
endif ()
