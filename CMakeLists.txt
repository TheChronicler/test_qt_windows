cmake_minimum_required(VERSION 3.25.2)
project(untitled)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)

string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)

message(STATUS "SDL2_INCLUDE_DIRS = ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2_LIBRARIES = ${SDL2_LIBRARIES}")

#set(WHISPER_CLBLAST ON)
#set(WHISPER_CUBLAS ON)

add_subdirectory(whisper.cpp)
include_directories(whisper.cpp/)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 6.5.0 COMPONENTS
#find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)



add_executable(test_main
	common.h
	common.cpp
	common-sdl.h
	common-sdl.cpp
	dr_wav.h
	main.cpp)

target_link_libraries(test_main PRIVATE
        whisper
        ${SDL2_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        Qt::Core
        Qt::Gui
        Qt::Widgets)

