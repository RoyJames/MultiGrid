cmake_minimum_required(VERSION 3.3.1)
project(multigrid)


# Set a default build type if none was specified
if( NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES )
	message( STATUS "Setting build type to 'debug' as none was specified." )
	set( CMAKE_BUILD_TYPE debug CACHE STRING "Choose the type of build." FORCE )
	# Set the possible values of build type for cmake-gui
	set_property( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "debug" "release" ) 
endif()

#set build directory
set(BUILD_DIRECTORY ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-deprecated")
set( CMAKE_CXX_FLAGS_DEBUG "-g" )
set( CMAKE_CXX_FLAGS_RELEASE "-O3 -Ofast -DNDEBUG" )

message( "Path: " ${CMAKE_SOURCE_DIR} )

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/DTSource
			${CMAKE_SOURCE_DIR}
)

# Add link directoris
link_directories( ${CMAKE_SOURCE_DIR}/DTSource
)

#set( SOURCEFILES src/main.cpp)

add_executable( multigrid main.cpp )

set( CMAKE_SHARED_LIBRARY_PREFIX "" )

set( THREADS_PREFER_PTHREAD_FLAG ON )

find_package( Threads REQUIRED )

set( EXTERNAL_LIBS
		DT
		lapack
		Threads::Threads )

target_link_libraries( multigrid ${EXTERNAL_LIBS}
                                 boost_program_options
)

set_target_properties( multigrid PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIRECTORY})


message(STATUS "CMAKE_SYSTEM_NAME          = ${CMAKE_SYSTEM_NAME}"         )
message(STATUS "CMAKE_CXX_COMPILER_ID      = ${CMAKE_CXX_COMPILER_ID}"     )
message(STATUS "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_COMPILER_IS_GNUCXX   = ${CMAKE_COMPILER_IS_GNUCXX}"  )
