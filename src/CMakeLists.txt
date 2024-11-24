# add_library(termination_handler SHARED termination_handler.cpp)
# target_include_directories(termination_handler PUBLIC ROOT_INCLUDE_DIRS ${PROJECT_SOURCE_DIR})
# target_link_libraries(termination_handler PUBLIC ROOT::Core)
# set_target_properties(
# 	termination_handler PROPERTIES
# 	CXX_STANDARD_REQUIRED ON
# 	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
# 	POSITION_INDEPENDENT_CODE ON
# )

add_library(online_data_receiver online_data_receiver.cpp)
target_include_directories(
	online_data_receiver PUBLIC ${PROJECT_SOURCE_DIR} ROOT_INCLUDE_DIRS
)
target_link_libraries(
	online_data_receiver PUBLIC
	iceoryx_binding_c::iceoryx_binding_c
)
target_compile_options(
	online_data_receiver PUBLIC ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS}
)
set_target_properties(
	online_data_receiver PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)