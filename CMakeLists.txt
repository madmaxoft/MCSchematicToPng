
cmake_minimum_required (VERSION 3.2)

project (SchematicToPng)

# Without this, the MSVC variable isn't defined for MSVC builds ( http://www.cmake.org/pipermail/cmake/2011-November/047130.html )
enable_language(CXX C)

set (CMAKE_CXX_STANDARD 11)

# Check that the libraries are present:
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/lib/libpng/CMakeLists.txt)
	message(FATAL_ERROR "libpng is missing in folder lib/libpng. Have you initialized the submodules / downloaded the extra libraries?")
endif()
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/lib/pngpp/png.hpp)
	message(FATAL_ERROR "png++ is missing in folder lib/pngpp. Have you initialized the submodules / downloaded the extra libraries?")
endif()
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/lib/zlib/CMakeLists.txt)
	message(FATAL_ERROR "zlib is missing in folder lib/zlib. Have you initialized the submodules / downloaded the extra libraries?")
endif()
if (NOT EXISTS ${CMAKE_SOURCE_DIR}/lib/jsoncpp/CMakeLists.txt)
	message(FATAL_ERROR "jsoncpp is missing in folder lib/jsoncpp. Have you initialized the submodules / downloaded the extra libraries?")
endif()

# Use static CRT in MSVC builds:
if (MSVC)
	string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
	string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}")
	string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}")
	string(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG}")
endif()

# Use Link-Time Code Generation in MSVC release builds:
if (MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /GL")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}    /LTCG")
	set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
	set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /LTCG")
endif()

# Make build use multiple threads under MSVC:
if (MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MP")
endif()

# GCC needs flags to enable C++11 and pthread:
if (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11 -pthread")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -pthread")
endif()

# Clang needs flags to enable C++11 and pthread:
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11 -pthread")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11 -pthread")
endif()

# CMake 3.5.2+ warns when getting the LOCATION property from a target, because it may not be available at generate-time under some special circumstances
# We need to read the zlib's LOCATION so that we can point libPNG's ZLIB support there, and we know it's a safe operation, hence we disable the warning:
if (POLICY CMP0026)
	cmake_policy(SET CMP0026 OLD)
endif()

# Include the libraries:
include_directories("${CMAKE_CURRENT_BINARY_DIR}/lib/zlib")
add_subdirectory(lib/zlib EXCLUDE_FROM_ALL)

get_target_property(ZLIB_LIBRARY zlibstatic LIBRARY_OUTPUT_NAME)
get_target_property(ZLIB_LIBRARY zlibstatic LOCATION)
set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib")
set(PNG_SHARED OFF CACHE BOOL "Build shared lib"   FORCE)
set(PNG_STATIC ON CACHE BOOL  "Build static lib"   FORCE)
set(PNG_TESTS  OFF CACHE BOOL "Build libpng tests" FORCE)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/lib/libpng")
add_subdirectory(lib/libpng EXCLUDE_FROM_ALL)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/libpng")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/pngpp")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/jsoncpp/include")
add_subdirectory(lib/jsoncpp EXCLUDE_FROM_ALL)

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

function(flatten_files arg1)
	set(res "")
	foreach(f ${${arg1}})
		get_filename_component(f ${f} ABSOLUTE)
		list(APPEND res ${f})
	endforeach()
	set(${arg1} "${res}" PARENT_SCOPE)
endfunction()

flatten_files(SHARED_SRC)
flatten_files(SHARED_HDR)
source_group("Shared" FILES ${SHARED_SRC} ${SHARED_HDR})

set(SHARED_OSS_SRC
	src/Shared/OSSupport/CriticalSection.cpp
	src/Shared/OSSupport/Errors.cpp
	src/Shared/OSSupport/Event.cpp
	src/Shared/OSSupport/File.cpp
	src/Shared/OSSupport/GZipFile.cpp
	src/Shared/OSSupport/IsThread.cpp
	src/Shared/OSSupport/StackTrace.cpp
)

set(SHARED_OSS_HDR
	src/Shared/OSSupport/CriticalSection.h
	src/Shared/OSSupport/Errors.h
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
	src/BlockColors.cpp
	src/BlockImage.cpp
	src/Globals.cpp
	src/InputStream.cpp
	src/JsonNet.cpp
	src/Marker.cpp
	src/PngExporter.cpp
	src/SchematicToPng.cpp
)
set(HEADERS
	src/BlockColors.h
	src/BlockImage.h
	src/Globals.h
	src/InputStream.h
	src/JsonNet.h
	src/Marker.h
	src/PngExporter.h
	src/SchematicToPng.h
)

source_group("" FILES ${SOURCES} ${HEADERS})

# Set include paths to the used libraries:
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/Shared")

if (MSVC)
	add_definitions("/D_CRT_SECURE_NO_WARNINGS")
endif()

add_executable(SchematicToPng
	${SOURCES}
	${HEADERS}
	${SHARED_SRC}
	${SHARED_HDR}
	${SHARED_OSS_SRC}
	${SHARED_OSS_HDR}
)

target_link_libraries(SchematicToPng zlibstatic png15_static jsoncpp_lib_static)

# Enable the support for solution folders in MSVC
if (MSVC)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

set_target_properties(
	zlibstatic
	png15_static
	jsoncpp_lib_static
	PROPERTIES FOLDER Lib
)
