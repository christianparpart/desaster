
add_definitions(-std=c++0x -pthread)
link_libraries(pthread)

add_executable(iterate-network-interfaces iterate-network-interfaces.cpp)
add_executable(t t.cpp)
add_executable(udp-broadcaster udp-broadcaster.cpp)
add_executable(udp-receiver udp-receiver.cpp)
