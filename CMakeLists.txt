cmake_minimum_required(VERSION 3.0)

project(aflnw)

FILE(GLOB SRC_FILES "src/*.c" "src/*.h")

# for debug
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

ADD_EXECUTABLE(aflnw ${SRC_FILES})
