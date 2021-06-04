project(openffb C)

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# Set application to GUI type for Win32 and MacOS
if(WIN32)
  set(GUI_TYPE WIN32)
endif(WIN32)
if(APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)
endif(APPLE)

# global variables
set(openffb_INCLUDE_DIR
  ${CMAKE_SOURCE_DIR}/include
)

set(openffb_TARGET_DIR
  ${CMAKE_BINARY_DIR}/target
)

set(openffb_LIB_DIR
  ${CMAKE_BINARY_DIR}/lib
)

set(APPLICATION_NAME ${PROJECT_NAME})

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

# disallow in-source build
include(MacroEnsureOutOfSourceBuild)
macro_ensure_out_of_source_build("${PROJECT_NAME} requires an out of source build. Please create a separate build directory and run 'cmake /path/to/${PROJECT_NAME} [options]' there.")

# Define output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${openffb_TARGET_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${openffb_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${openffb_LIB_DIR})


include_directories(${THREAD_INLCUDE_DIRS} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR})

FILE( GLOB_RECURSE Sources src/*.c )
add_executable( ${PROJECT_NAME} ${Sources} )
# add_subdirectory(src)

#find_package(SDL2 REQUIRED)
#target_link_libraries(openffb ${SDL2_LIBRARY})
target_link_libraries(openffb udev)

install(TARGETS ${PROJECT_NAME}
		COMPONENT ${PROJECT_NAME}
		RUNTIME DESTINATION "bin/"
		LIBRARY DESTINATION "lib/"
)

install(FILES ${CMAKE_SOURCE_DIR}/docs/openffb.service
		COMPONENT ${PROJECT_NAME}
		DESTINATION "/etc/systemd/system/"
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/docs/openffb
		COMPONENT ${PROJECT_NAME}
		DESTINATION "/etc"
)

SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "fbernard1@hotmail.com") #required
INCLUDE(CPack)
