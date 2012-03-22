# systemd
set(SD_SOURCES sd-daemon.c)
set(SD_HEADERS sd-daemon.h)
add_library(sd-daemon STATIC ${SD_SOURCES})
set(SD_LIBRARIES sd-daemon)

# desasterd
add_executable(desasterd desasterd.cpp Server.cpp)
set_target_properties(desasterd PROPERTIES COMPILE_FLAGS "-std=c++0x -pthread")
target_link_libraries(desasterd ${SD_LIBRARIES} ${EV_LIBRARIES} pthread)
