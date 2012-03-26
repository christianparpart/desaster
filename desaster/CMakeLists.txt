# systemd
set(SD_SOURCES sd-daemon.c)
set(SD_HEADERS sd-daemon.h)
add_library(sd-daemon STATIC ${SD_SOURCES})
set(SD_LIBRARIES sd-daemon)

add_definitions(${XZERO_CFLAGS})
link_directories(${XZERO_LIBRARY_DIRS})

# desaster
add_executable(desaster main.cpp Server.cpp Worker.cpp Job.cpp Pipe.cpp qdisc.cpp)
set_target_properties(desaster PROPERTIES COMPILE_FLAGS "-std=c++0x -pthread")
target_link_libraries(desaster ${SD_LIBRARIES} ${XZERO_LIBRARIES} ${EV_LIBRARIES} pthread)
