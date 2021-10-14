cmake_minimum_required(VERSION 3.1)

project(lang_play)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb -DDBG_MACRO_NO_WARNING")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

SET(SRC_FILE "play.cpp"
                       )

add_executable(${PROJECT_NAME} ${SRC_FILE})

include_directories("third_party/dbg-macro/")