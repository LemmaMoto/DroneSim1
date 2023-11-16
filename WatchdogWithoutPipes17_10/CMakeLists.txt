cmake_minimum_required(VERSION 3.2)

project("Watchdog")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

find_package(Curses REQUIRED)

include_directories("./headers")
set(WRAPSRC utility/wrapfunc.c)

add_executable(watchdog watchdog.c ${WRAPSRC})
add_executable(child child.c ${WRAPSRC})

target_link_libraries(watchdog ${CURSES_LIBRARIES} m)
target_link_libraries(child ${CURSES_LIBRARIES} m)
