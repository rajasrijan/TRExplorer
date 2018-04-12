cmake_minimum_required(VERSION 3.5)
PROJECT(MeshPlugin)

add_library(MeshPlugin MODULE MeshPlugin.cpp tinyxml2.cpp)

include_directories(${COMMON_INCLUDE_DIRS})

set_property(TARGET MeshPlugin PROPERTY CXX_STANDARD 14)

install(
  TARGETS MeshPlugin
  LIBRARY DESTINATION .
  )