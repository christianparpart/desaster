add_executable(desaster-cli main.cpp)
set_target_properties(desaster-cli PROPERTIES COMPILE_FLAGS "-std=c++0x -pthread")
