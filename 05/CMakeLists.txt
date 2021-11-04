cmake_minimum_required(VERSION 3.1)

project(lang_play)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb -DDBG_MACRO_NO_WARNING")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")


SET(MAIN_FILE  "play.cpp")
SET(SRC_FILE  "common.cpp"
              "scanner.cpp"
              "types.cpp"
              "symbol.cpp"
              "ast.cpp"
              "parser.cpp"
                       )
add_executable(${PROJECT_NAME} ${SRC_FILE} ${MAIN_FILE})
include_directories("third_party/dbg-macro/")

set(ENABLE_TESTS ON)

if (ENABLE_TESTS)

    file(GLOB GTEST_FILES  "third_party/googletest-1.10.0/googletest/src/gtest-all.cc"
                            "third_party/googletest-1.10.0/googletest/src/gmock-all.cc"
                        )
    file(GLOB TESTCASE_FILES  "tests/*.cpp")

    include_directories("/")
    include_directories("third_party/dbg-macro/")
    include_directories("third_party/googletest-1.10.0/googletest/")
    include_directories("third_party/googletest-1.10.0/googletest/include/")
    include_directories("third_party/googletest-1.10.0/googlemock/")
    include_directories("third_party/googletest-1.10.0/googlemock/include/")
    add_executable(tests-${PROJECT_NAME} ${GTEST_FILES} ${SRC_FILE} ${TESTCASE_FILES})


endif()
