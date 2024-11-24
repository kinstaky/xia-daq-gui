# Require the cmake minimum version.
cmake_minimum_required(VERSION 3.10)


# Set project properties.
project(
	XiaDaqGui
	VERSION 1.0.0
	DESCRIPTION "A project for XIA DAQ GUI, edited from GDDAQ."
	LANGUAGES CXX C
)


# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if (EXISTS "${LOC_PATH}")
	message(FATAL_ERROR "You cannot build in a source directory (or any directory with"
						"CmakeLists.txt file). Please make a build subdirectory. Feel free"
						"to remove CMakeCache.txt and CMakeFiles.")
endif()

# Set the build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()


# Only do these if this is the main project, and not if it is included
# through add_subdirectory
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

	# Let's ensure -std=c++xx instead of -std=g++xx
	set(CMAKE_CXX_EXTENSIONS OFF)

	# Set the default cxx standard
	set(CMAKE_CXX_STANDARD 17)

	# Set default compile options
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

	# Let's nicely support folders in IDE's
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

endif()

# find root
find_package(ROOT 6.20 CONFIG REQUIRED)

include(GNUInstallDirs)

# find iceoryx
find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_hoofs CONFIG REQUIRED)
find_package(iceoryx_binding_c CONFIG REQUIRED)
get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPackageHelper)
include(IceoryxPlatform)

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(bin)
add_subdirectory(examples)