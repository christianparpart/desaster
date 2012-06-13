add_executable(desaster-cli main.cpp
	${CMAKE_SOURCE_DIR}/desaster/NetMessage.cpp
	${CMAKE_SOURCE_DIR}/desaster/Buffer.cpp
	${CMAKE_SOURCE_DIR}/desaster/Logging.cpp
	${CMAKE_SOURCE_DIR}/desaster/CLI.cpp
)
set_target_properties(desaster-cli PROPERTIES COMPILE_FLAGS "-std=c++0x")
target_link_libraries(desaster-cli ${EV_LIBRARIES} readline)
