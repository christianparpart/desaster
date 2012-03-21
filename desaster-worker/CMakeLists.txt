# desaster-worker
add_executable(desaster-worker desaster-worker.cpp)
set_target_properties(desaster-worker PROPERTIES COMPILE_FLAGS "-std=c++0x -pthread")
target_link_libraries(desaster-worker ${SD_LIBRARIES} ${EV_LIBRARIES} pthread)
