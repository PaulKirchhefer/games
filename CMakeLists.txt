#adapted from STAMINA!
cmake_minimum_required (VERSION 2.6)
project (Games)

set(CMAKE_CXX_FLAGS 
	"${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-DUNIX)

#set(CMAKE_BUILD_TYPE Debug)

set (EXTRA_LIBS
)

include_directories(
	"stamina"
	"games"
)

include (Sources.cmake)

set (ALL_SOURCES ${SOURCES}
Sources.cmake
)

# ADD HEADERS FILES to project (useful in XCode / MSVC)
foreach (SOURCE ${ALL_SOURCES})
	set (FILENAME_PREFIX)
	set (FILENAME_PATH)
	GET_FILENAME_COMPONENT(FILENAME_PREFIX ${SOURCE} NAME_WE)
	GET_FILENAME_COMPONENT(FILENAME_PATH ${SOURCE} PATH)
	
	if (FILENAME_PATH STREQUAL "")
		set (FILENAME ${FILENAME_PREFIX})
	else ()
		set (FILENAME ${FILENAME_PATH}/${FILENAME_PREFIX})
	endif()
	
	if(EXISTS "${PROJECT_SOURCE_DIR}/${FILENAME}.h")
        set (ALL_HEADERS ${ALL_HEADERS} "${FILENAME}.h")
	endif(EXISTS "${PROJECT_SOURCE_DIR}/${FILENAME}.h")
	if(EXISTS "${PROJECT_SOURCE_DIR}/${FILENAME}.hpp")
        set (ALL_HEADERS ${ALL_HEADERS} "${FILENAME}.hpp")
	endif(EXISTS "${PROJECT_SOURCE_DIR}/${FILENAME}.hpp")
endforeach (SOURCE)

add_executable (games ${ALL_SOURCES} ${ALL_HEADERS} games/main.cpp)

