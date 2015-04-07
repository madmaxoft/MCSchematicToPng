
cmake_minimum_required (VERSION 2.6)

project (SchematicToPng)

# Without this, the MSVC variable isn't defined for MSVC builds ( http://www.cmake.org/pipermail/cmake/2011-November/047130.html )
enable_language(CXX C)


function(flatten_files arg1)
	set(res "")
	foreach(f ${${arg1}})
		get_filename_component(f ${f} ABSOLUTE)
		list(APPEND res ${f})
	endforeach()
	set(${arg1} "${res}" PARENT_SCOPE)
endfunction()


# Include the libraries:
add_subdirectory(lib/zlib EXCLUDE_FROM_ALL)

get_target_property(ZLIB_LIBRARY zlibstatic LIBRARY_OUTPUT_NAME)
get_target_property(ZLIB_LIBRARY zlibstatic LOCATION)
message("ZLIB_LIBRARY = " ${ZLIB_LIBRARY})

set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib")
set(PNG_SHARED OFF CACHE BOOL "Build shared lib"   FORCE)
set(PNG_STATIC ON CACHE BOOL  "Build static lib"   FORCE)
set(PNG_TESTS  OFF CACHE BOOL "Build libpng tests" FORCE)
add_subdirectory(lib/libpng EXCLUDE_FROM_ALL)
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/lib/libpng")
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/lib/pngpp")
include_directories(SYSTEM "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/lib/libpng")

# Include the shared files:
set(SHARED_SRC
	src/Shared/LoggerListeners.cpp
	src/Shared/Logger.cpp
	src/Shared/StringCompression.cpp
	src/Shared/StringUtils.cpp
	src/Shared/WorldStorage/FastNBT.cpp
)
set(SHARED_HDR
	src/Shared/Endianness.h
	src/Shared/LoggerListeners.h
	src/Shared/Logger.h
	src/Shared/StringUtils.h
	src/Shared/StringCompression.h
	src/Shared/WorldStorage/FastNBT.h
)

flatten_files(SHARED_SRC)
flatten_files(SHARED_HDR)
source_group("Shared" FILES ${SHARED_SRC} ${SHARED_HDR})

set(SHARED_OSS_SRC
	src/Shared/OSSupport/CriticalSection.cpp
	src/Shared/OSSupport/Event.cpp
	src/Shared/OSSupport/File.cpp
	src/Shared/OSSupport/GZipFile.cpp
	src/Shared/OSSupport/IsThread.cpp
	src/Shared/OSSupport/StackTrace.cpp
)

set(SHARED_OSS_HDR
	src/Shared/OSSupport/CriticalSection.h
	src/Shared/OSSupport/Event.h
	src/Shared/OSSupport/File.h
	src/Shared/OSSupport/GZipFile.h
	src/Shared/OSSupport/IsThread.h
	src/Shared/OSSupport/StackTrace.h
)

if(WIN32)
	list (APPEND SHARED_OSS_SRC src/Shared/StackWalker.cpp)
	list (APPEND SHARED_OSS_HDR src/Shared/StackWalker.h)
endif()

flatten_files(SHARED_OSS_SRC)
flatten_files(SHARED_OSS_HDR)

source_group("Shared\\OSSupport" FILES ${SHARED_OSS_SRC} ${SHARED_OSS_HDR})



# Include the main source files:
set(SOURCES
	src/BlockImage.cpp
	src/Globals.cpp
	src/PngExporter.cpp
	src/SchematicToPng.cpp
)
set(HEADERS
	src/BlockImage.h
	src/Globals.h
	src/PngExporter.h
	src/SchematicToPng.h
)

source_group("" FILES ${SOURCES} ${HEADERS})

# Set include paths to the used libraries:
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/Shared")

add_executable(SchematicToPng
	${SOURCES}
	${HEADERS}
	${SHARED_SRC}
	${SHARED_HDR}
	${SHARED_OSS_SRC}
	${SHARED_OSS_HDR}
)

target_link_libraries(SchematicToPng zlibstatic png15_static)
