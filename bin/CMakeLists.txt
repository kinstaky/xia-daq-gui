# add_executable(gddaq gddaq.cpp)
# target_include_directories(
# 	gddaq PUBLIC ROOT_INCLUDE_DIRS
# )
# target_link_directories(gddaq PRIVATE $ENV{PLX_SDK_DIR}/PlxApi/Library)
# target_link_libraries(
# 	gddaq PRIVATE
# 	gui
# 	crypto rt fftw3
# 	Pixie16App Pixie16Sys PlxApi
# 	iceoryx_posh::iceoryx_posh
# )
# set_target_properties(
# 	gddaq PROPERTIES
# 	CXX_STANDARD_REQUIRED ON
# 	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
# 	POSITION_INDEPENDENT_CODE ON
# )
# ROOT_GENERATE_DICTIONARY(
# 	MainDict ${PROJECT_SOURCE_DIR}/external/daq/MainFrame.hh
# 	MODULE gddaq
# 	LINKDEF ${PROJECT_SOURCE_DIR}/external/daq/MainLinkdef.h
# )


# add_executable(online online.cpp)
# target_include_directories(
# 	online PRIVATE ${PROJECT_SOURCE_DIR} ROOT_INCLUDE_DIRS
# )
# target_link_libraries(
# 	online PRIVATE
# 	termination-handler
# 	iceoryx_binding_c::iceoryx_binding_c iceoryx_posh::iceoryx_posh
# 	ROOT::Core ROOT::MathCore ROOT::Hist ROOT::Graf ROOT::Gui ROOT::Rint
# )
# target_compile_options(
# 	online PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS}
# )
# set_target_properties(
# 	online PROPERTIES
# 	CXX_STANDARD_REQUIRED ON
# 	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
# 	POSITION_INDEPENDENT_CODE ON
# )

# ROOT_GENERATE_DICTIONARY(
# 	online_dict ${PROJECT_SOURCE_DIR}/include/termination_handler.h
# 	MODULE online
# 	LINKDEF ${PROJECT_SOURCE_DIR}/include/linkdef.h
# )
