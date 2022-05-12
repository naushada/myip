cmake_minimum_required(VERSION 3.16.3)
project(dynip)

include_directories(inc)
include_directories(/usr/local/ACE_TAO-7.0.0/include)

link_directories(/usr/local/ACE_TAO-7.0.0/lib)

#Adding Complier flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall -Wextra")

#However, the file(GLOB...) allows for wildcard additions:
file(GLOB SOURCES "src/*.cc")

add_executable(uniip ${SOURCES})
target_link_libraries(uniip pthread)
target_link_libraries(uniip ACE)
target_link_libraries(uniip ACE_SSL)
target_link_libraries(uniip ssl crypto)

