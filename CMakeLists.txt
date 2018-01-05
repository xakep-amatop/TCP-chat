cmake_minimum_required (VERSION 2.6)

set(PROJECT_TCP_CHAT tcp_chat)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Actually search for the library
find_package(Readline)
find_package(Ncurses)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/common ${CMAKE_CURRENT_SOURCE_DIR}/src_client ${CMAKE_CURRENT_SOURCE_DIR}/src_server /usr/local/include /usr/local/lib ${Readline_INCLUDE_DIR} ${Ncurses_INCLUDE_DIR} )

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -g -O0")

set(SOURCES_COMMON ${CMAKE_CURRENT_SOURCE_DIR}/parse_args.cpp)
set(SOURCES_SERVER ${CMAKE_CURRENT_SOURCE_DIR}/src_server/server.cpp)
set(SOURCES_CLIENT ${CMAKE_CURRENT_SOURCE_DIR}/src_client/client.cpp)

add_executable(${PROJECT_TCP_CHAT} tcp_chat_cs.cpp ${SOURCES_SERVER} ${SOURCES_CLIENT} ${SOURCES_COMMON})

target_link_libraries(${PROJECT_TCP_CHAT} ${Readline_LIBRARY} ${Ncurses_LIBRARY} -lpthread)
