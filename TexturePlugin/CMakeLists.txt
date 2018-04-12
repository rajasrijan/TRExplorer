cmake_minimum_required(VERSION 3.5)
PROJECT(TexturePlugin)

add_library(TexturePlugin MODULE TexturePlugin.cpp TextureHeader.cpp DDSLoader.cpp)

include_directories(${COMMON_INCLUDE_DIRS})

set_property(TARGET TexturePlugin PROPERTY CXX_STANDARD 14)

install(
  TARGETS TexturePlugin
  LIBRARY DESTINATION .
  )