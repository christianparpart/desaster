# systemd
set(SD_SOURCES sd-daemon.c)
set(SD_HEADERS sd-daemon.h)
add_library(sd-daemon STATIC ${SD_SOURCES})
set(SD_LIBRARIES sd-daemon)

add_definitions(${XZERO_CFLAGS})
link_directories(${XZERO_LIBRARY_DIRS})

# desaster
set(shell_worker_SOURCES shell/ShellWorker.cpp shell/ShellJob.cpp shell/ShellModule.cpp)
set(shell_worker_HEADERS shell/ShellWorker.h shell/ShellJob.h shell/ShellModule.h)

set(core_SOURCES Server.cpp Module.cpp Worker.cpp Job.cpp Pipe.cpp qdisc.cpp)
set(core_HEADERS Server.h Module.h Worker.h Job.h Pipe.h qdisc.h)

add_executable(desaster main.cpp ${core_SOURCES} ${shell_worker_SOURCES})
set_target_properties(desaster PROPERTIES COMPILE_FLAGS "-std=c++0x -pthread")
target_link_libraries(desaster ${SD_LIBRARIES} ${XZERO_LIBRARIES} ${EV_LIBRARIES} pthread)
