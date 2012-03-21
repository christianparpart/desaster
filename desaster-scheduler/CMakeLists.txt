# desaster-scheduler
add_executable(desaster-scheduler desaster-scheduler.cpp)
set_target_properties(desaster-scheduler PROPERTIES COMPILE_FLAGS "-std=c++0x -pthread")
target_link_libraries(desaster-scheduler ${SD_LIBRARIES} ${EV_LIBRARIES} pthread)
