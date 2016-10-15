set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 2.8)

project(containit C)

if (${UNIX})
    add_definitions("-Wall")
    add_definitions("-Werror")
    add_definitions("-Wextra")
    add_definitions("-O3")
    add_definitions("-D_GNU_SOURCE")
endif ()

add_executable(${PROJECT_NAME} ${PROJECT_NAME}.c)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
