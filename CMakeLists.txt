cmake_minimum_required(VERSION 3.5)
project(CharUDPMPV)

set(CMAKE_CXX_STANDARD 17)

add_executable(CharUDPMPV
        src/main.cpp
        src/UdpComm.cpp
        src/UdpComm.h
        src/json.hpp
        src/Player.cpp
        src/Player.h
        src/Controller.cpp
        src/Controller.h
        src/RandomizedSender.cpp
        src/RandomizedSender.h
)

if (WIN32)
    message(STATUS "Configuring for Windows...")

    set(MPV_ROOT "${CMAKE_SOURCE_DIR}/external/mpv")
    set(MPV_INCLUDE_DIR "${MPV_ROOT}/include")
    set(MPV_LIB_DIR "${MPV_ROOT}")

    include_directories(${MPV_INCLUDE_DIR})
    link_directories(${MPV_LIB_DIR})

    target_link_libraries(CharUDPMPV "${MPV_ROOT}/libmpv.dll.a")

    add_custom_command(TARGET CharUDPMPV POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MPV_ROOT}/libmpv-2.dll"
            $<TARGET_FILE_DIR:CharUDPMPV>
    )

else()
    message(STATUS "Configuring for Linux/Unix...")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPV REQUIRED mpv)

    include_directories(${MPV_INCLUDE_DIRS})
    link_directories(${MPV_LIBRARY_DIRS})
    target_link_libraries(CharUDPMPV ${MPV_LIBRARIES})
endif()
