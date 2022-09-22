project( osclassifier )

cmake_minimum_required(VERSION 3.1)

if(NOT POLICY CMP0070 AND POLICY CMP0053)
  # Only set CMP0053 to OLD with CMake<3.10, otherwise there is a warning.
  cmake_policy(SET CMP0053 OLD)
endif()

if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

set( CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true )
 
if ( COMMAND cmake_policy )
  cmake_policy( SET CMP0003 NEW )  
endif()

# GDAL
find_package( GDAL 3.0 )

if ( NOT GDAL_FOUND )
  message(SEND_ERROR "prepair requires the GDAL library")
endif()

include_directories( ${GDAL_INCLUDE_DIR} )

# Creating entries for target: osclassifier
# ############################

add_executable( osclassifier main.cpp )

set_property(TARGET osclassifier PROPERTY CXX_STANDARD 11)

# Link to CGAL and third-party libraries
target_link_libraries( osclassifier ${GDAL_LIBRARY} )
