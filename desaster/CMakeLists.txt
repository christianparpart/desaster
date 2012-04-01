# systemd
set(SD_SOURCES sd-daemon.c)
set(SD_HEADERS sd-daemon.h)
add_library(sd-daemon STATIC ${SD_SOURCES})
set(SD_LIBRARIES sd-daemon)

# qdisc
set(QDISC_SOURCES qdisc.cpp)
set(QDISC_HEADERS qdisc.h)
add_library(qdisc STATIC ${QDISC_SOURCES})
set_target_properties(qdisc PROPERTIES COMPILE_FLAGS "-std=c++0x")
set(QDISC_LIBRARIES qdisc)

add_definitions(${XZERO_CFLAGS})
link_directories(${XZERO_LIBRARY_DIRS})

# desaster
set(shell_worker_SOURCES shell/ShellModule.cpp shell/ShellJob.cpp)
set(shell_worker_HEADERS shell/ShellModule.h shell/ShellJob.h)

set(core_SOURCES Server.cpp Connection.cpp Module.cpp Queue.cpp Job.cpp Pipe.cpp)
set(core_HEADERS Server.h Connection.h Module.h Queue.h Job.h Pipe.h)

add_executable(desaster main.cpp ${core_SOURCES} ${shell_worker_SOURCES})
set_target_properties(desaster PROPERTIES COMPILE_FLAGS "-std=c++0x -pthread")
target_link_libraries(desaster ${QDISC_LIBRARIES} ${SD_LIBRARIES} ${XZERO_LIBRARIES} ${EV_LIBRARIES} pthread)
