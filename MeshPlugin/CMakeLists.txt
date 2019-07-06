INCLUDE(FindPkgConfig)
cmake_minimum_required(VERSION 3.5)
PROJECT(MeshPlugin)

add_library(MeshPlugin MODULE MeshPlugin.cpp tinyxml2.cpp)

include_directories(${COMMON_INCLUDE_DIRS})

pkg_check_modules(assimp REQUIRED assimp)
IF(assimp_FOUND)
  IF(assimp_INCLUDE_DIRS)
	include_directories("${assimp_INCLUDE_DIRS}")
  ENDIF(assimp_INCLUDE_DIRS)
  # and for each of your dependant executable/library targets:
  TARGET_LINK_LIBRARIES(MeshPlugin ${assimp_LIBRARIES})
ELSE(assimp_FOUND)
  # For convenience. When we cannot continue, inform the user
  MESSAGE("Assimp not found!")
ENDIF(assimp_FOUND)
pkg_check_modules(glm REQUIRED glm)
IF (glm_FOUND)
    if (NOT "${glm_INCLUDE_DIRS}" STREQUAL "")
        include_directories("${glm_INCLUDE_DIRS}")
    ENDIF ()
    # and for each of your dependant executable/library targets:
    TARGET_LINK_LIBRARIES(MeshPlugin ${glm_LIBRARIES})
ELSE (glm_FOUND)
    # For convenience. When we cannot continue, inform the user
    MESSAGE("glm not found!")
ENDIF (glm_FOUND)

set_property(TARGET MeshPlugin PROPERTY CXX_STANDARD 17)

add_dependencies(TRExplorer MeshPlugin)

install(
  TARGETS MeshPlugin
  LIBRARY DESTINATION .
  )