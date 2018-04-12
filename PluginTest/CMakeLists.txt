cmake_minimum_required(VERSION 3.5)
PROJECT(PluginTest)

ADD_EXECUTABLE(plugintest PluginTest.cpp)
TARGET_LINK_LIBRARIES(plugintest ${CMAKE_DL_LIBS})
include_directories(${COMMON_INCLUDE_DIRS})

set_property(TARGET plugintest PROPERTY CXX_STANDARD 14)