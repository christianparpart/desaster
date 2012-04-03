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

# desaster
set(core_SOURCES Server.cpp Module.cpp Worker.cpp Queue.cpp Job.cpp
	Buffer.cpp NetMessage.cpp Pipe.cpp Logging.cpp)
set(core_HEADERS Server.h Module.h Worker.h Queue.h Job.h
	NetMessage.h BufferRef.h Buffer.h Pipe.h Logging.h)

set(network_adapter_SOURCES NetworkAdapter.cpp)
set(network_adapter_HEADERS NetworkAdapter.h)

set(cluster_adapter_SOURCES ClusterAdapter.cpp)
set(cluster_adapter_HEADERS ClusterAdapter.h)

set(shell_worker_SOURCES shell/ShellModule.cpp shell/ShellWorker.cpp)
set(shell_worker_HEADERS shell/ShellModule.h shell/ShellWorker.h)

add_executable(desaster main.cpp ${core_SOURCES}
	${network_adapter_SOURCES} ${cluster_adapter_SOURCES} ${shell_worker_SOURCES})
set_target_properties(desaster PROPERTIES COMPILE_FLAGS "-std=c++0x -pthread")
target_link_libraries(desaster ${QDISC_LIBRARIES} ${SD_LIBRARIES} ${EV_LIBRARIES} pthread)
