cmake_minimum_required(VERSION 3.9)
set(PROJECT_NAME "muxev")
project(${PROJECT_NAME})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_FLAGS}")

add_executable(${PROJECT_NAME}
    "src/main.cpp"
    "src/switch.cpp"
    "src/router.cpp"
    "src/evdev.cpp"
    "src/mouseinputdev.cpp"
    "src/kbdinputdev.cpp"
    "src/evinputdev.cpp"
    "src/socket.cpp"
    "src/udpsocket.cpp"
)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

