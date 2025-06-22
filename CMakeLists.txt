cmake_minimum_required(VERSION 4.0)

set(CMAKE_C_COMPILER "/usr/bin/clang-14")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-14")

project("number_guessing_enet")



set(OSBitness 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OSBitness 64)
endif()
set (FullOutputDir "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}${OSBitness}/${CMAKE_BUILD_TYPE}")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${FullOutputDir}/static_libs")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



include_directories(.)



include(FetchContent)

set(ENET_VERSION v1.3.18)
FetchContent_Declare(
    enet
    GIT_REPOSITORY "https://github.com/lsalzman/enet.git"
    GIT_TAG        "${ENET_VERSION}"
)
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(enet)



set(CLIENT_PROJECT_NAME "client_${PROJECT_NAME}")

add_executable(${CLIENT_PROJECT_NAME} client.cpp)
target_link_libraries(${CLIENT_PROJECT_NAME} PUBLIC enet)

set(SERVER_PROJECT_NAME "server_${PROJECT_NAME}")

add_executable(${SERVER_PROJECT_NAME} server.cpp)
target_link_libraries(${SERVER_PROJECT_NAME} PUBLIC enet)
