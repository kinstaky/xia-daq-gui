add_library(termination-handler SHARED termination-handler.cpp)
target_include_directories(termination-handler PUBLIC ROOT_INCLUDE_DIRS ${PROJECT_SOURCE_DIR})
target_link_libraries(termination-handler PUBLIC ROOT::Core)