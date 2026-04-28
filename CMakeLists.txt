cmake_minimum_required(VERSION 3.31)
project(Socket)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_WIN32_WINNT=0x0A00)

#set(CMAKE_FIND_DEBUG_MODE TRUE)
file(GLOB Headers "E:/Clion_Code/Socket/headers/*.h")
file(GLOB Sources "E:/Clion_Code/Socket/sources/*.cpp")
file(GLOB Sources_cc "E:/Clion_Code/Socket/sources/*.cc")
find_package(grpc CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

if (MSVC)
    add_compile_options(/utf-8)
endif()
add_executable(BoostText  # 这个是GateServer的启动项目
        ${Headers}
        ${Sources}
        ${Sources_cc}
)

# Boost 路径
target_include_directories(BoostText PRIVATE
        "E:/CLion 2024.3.1.1/Boost_MSVC"
        "E:/Clion_Code/Socket/libjson/include"
        "E:/MySQL-connector-c++/mysql-connector-c++-9.6.0-winx64/include"
)

target_link_directories(BoostText PRIVATE
        "E:/CLion 2024.3.1.1/Boost_MSVC/stage/lib"
        "E:/Clion_Code/Socket/libjson/lib"
        "E:/MySQL-connector-c++/mysql-connector-c++-9.6.0-winx64/lib64/vs14"
)

target_link_libraries(BoostText PRIVATE
        gRPC::grpc++
        protobuf::libprotobuf
        redis++::redis++
        rapidjson
        "E:/Clion_Code/Socket/libjson/lib/jsoncpp.lib"
        "E:/MySQL-connector-c++/mysql-connector-c++-9.6.0-winx64/lib64/vs14/mysqlcppconn.lib"
)



if (WIN32)

    add_custom_command(TARGET BoostText
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/config.ini
            $<TARGET_FILE_DIR:BoostText>
    )

    add_custom_command(TARGET BoostText
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "E:/MySQL-connector-c++/mysql-connector-c++-9.6.0-winx64/lib64/mysqlcppconn-10-vs14.dll"
            $<TARGET_FILE_DIR:BoostText>
    )

endif()
# add_executable(Socket_s hello_server_win.cpp)
# add_executable(Socket_c hello_client_win.cpp)
# add_executable(Test_echo_server echo_server.cpp)
# add_executable(Test_AsynNotiEcho_server AsynNotiEchoServ_win.cpp)
# add_executable(Test_OverlappedSend OverlappedSend.cpp)
# add_executable(Test_OverlappedRecv OverlappedRecv_win.cpp)
# add_executable(Test_echo_client echo_client.cpp)
# add_executable(Test_op_server op_server.cpp)
# add_executable(Test_op_client op_client.cpp)
# add_executable(Test_file_server file_server.cpp)
# add_executable(Test_file_client file_client.cpp)
# add_executable(Test_news_sender news_sender_win.cpp)
# add_executable(Test_news_receiver news_receiver_win.cpp)
# add_executable(Test_CmplRouEchoServ_win CmplRouEchoServ_win.cpp)
# add_executable(Test_StableEchoClnt_win StableEchoClnt_win.cpp)
# add_executable(Test_IOCPEchoServ_win IOCPEchoServ_win.cpp)
# add_executable(Test_IOCP_win IOCP.cpp)

# target_link_libraries(Socket_s ws2_32)
# target_link_libraries(Socket_c ws2_32)
# target_link_libraries(Test_echo_server ws2_32)
# target_link_libraries(Test_echo_client ws2_32)
# target_link_libraries(Test_op_server ws2_32)
# target_link_libraries(Test_op_client ws2_32)
# target_link_libraries(Test_file_client ws2_32)
# target_link_libraries(Test_file_server ws2_32)
# target_link_libraries(Test_news_sender ws2_32)
# target_link_libraries(Test_news_receiver ws2_32)
# target_link_libraries(Test_AsynNotiEcho_server ws2_32)
# target_link_libraries(Test_OverlappedSend ws2_32)
# target_link_libraries(Test_OverlappedRecv ws2_32)
# target_link_libraries(Test_CmplRouEchoServ_win ws2_32)
# target_link_libraries(Test_StableEchoClnt_win ws2_32)
# target_link_libraries(Test_IOCPEchoServ_win ws2_32)
# target_link_libraries(Test_IOCP_win ws2_32 mswsock)