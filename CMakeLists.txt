cmake_minimum_required(VERSION 3.5)
project(CharUDPMPV)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)

include_directories(${MPV_INCLUDE_DIRS})
link_directories(${MPV_LIBRARY_DIRS})

add_executable(CharUDPMPV
        src/main.cpp
        src/UdpComm.cpp
        src/UdpComm.h
        src/json.hpp
        src/Player.cpp
        src/Player.h
        src/Controller.cpp
        src/Controller.h)
target_link_libraries(CharUDPMPV ${MPV_LIBRARIES})
