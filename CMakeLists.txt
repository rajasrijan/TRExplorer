cmake_minimum_required(VERSION 3.5)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

PROJECT(TRExplorer)
SET(wxWidgets_USE_LIBS)
set(Plugin_Tester ON CACHE BOOL "Enable Plugin tester build")
set(Plugin_TexturePlugin ON CACHE BOOL "Enable Texture Plugin build")
set(Plugin_MeshPlugin ON CACHE BOOL "Enable Mesh Plugin build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/")

ADD_EXECUTABLE(TRExplorer TRExplorer/main.cpp TRExplorer/gui.cpp TRExplorer/patch.cpp TRExplorer/element.cpp
	 TRExplorer/thumbnailctrl.cpp TRExplorer/MyFrame.cpp TRExplorer/crc32.cpp TRExplorer/cdrm.cpp
	 TRExplorer/tiger.cpp  TRExplorer/DDSTex.cpp)

find_package(wxWidgets REQUIRED net gl core base aui)
IF(wxWidgets_FOUND)
  INCLUDE("${wxWidgets_USE_FILE}")
  # and for each of your dependant executable/library targets:
  TARGET_LINK_LIBRARIES(TRExplorer ${wxWidgets_LIBRARIES})
ELSE(wxWidgets_FOUND)
  # For convenience. When we cannot continue, inform the user
  MESSAGE("wxWidgets not found!")
ENDIF(wxWidgets_FOUND)

FIND_PACKAGE(ZLIB REQUIRED)
IF(ZLIB_FOUND)
  include_directories("${ZLIB_INCLUDE_DIRS}")
  # and for each of your dependant executable/library targets:
  TARGET_LINK_LIBRARIES(TRExplorer ${ZLIB_LIBRARIES})
ELSE(ZLIB_FOUND)
  # For convenience. When we cannot continue, inform the user
  MESSAGE("ZLIB not found!")
  SET(ZLIB_LIBRARY ${CMAKE_BINARY_DIR}/external/lib)
  SET(ZLIB_INCLUDE_DIR ${CMAKE_BINARY_DIR}/external/include)
  ExternalProject_Add(zlib
    PREFIX zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/external/ -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    )
ENDIF(ZLIB_FOUND)

FIND_PACKAGE(DevIL REQUIRED)
IF(IL_FOUND)
  include_directories("${IL_INCLUDE_DIR}")
  # and for each of your dependant executable/library targets:
  TARGET_LINK_LIBRARIES(TRExplorer ${ILUT_LIBRARIES} ${ILU_LIBRARIES} ${IL_LIBRARIES})
ELSE(IL_FOUND)
  # For convenience. When we cannot continue, inform the user
  MESSAGE("DevIL not found!")
ENDIF(IL_FOUND)

TARGET_LINK_LIBRARIES(TRExplorer ${CMAKE_DL_LIBS})

set(COMMON_INCLUDE_DIRS include)

include_directories(${COMMON_INCLUDE_DIRS})
    
configure_file("TRExplorer/filelist1.txt" "${CMAKE_CURRENT_BINARY_DIR}/filelist1.txt" COPYONLY)
configure_file("TRExplorer/filelist2.txt" "${CMAKE_CURRENT_BINARY_DIR}/filelist2.txt" COPYONLY)
configure_file("TRExplorer/PluginList.txt" "${CMAKE_CURRENT_BINARY_DIR}/PluginList.txt" COPYONLY)
configure_file("TRExplorer/NA.bmp" "${CMAKE_CURRENT_BINARY_DIR}/NA.bmp" COPYONLY)
configure_file("TRExplorer/MESH.bmp" "${CMAKE_CURRENT_BINARY_DIR}/MESH.bmp" COPYONLY)

IF(Plugin_Tester)
	add_subdirectory(PluginTest)
ENDIF(Plugin_Tester)
IF(Plugin_TexturePlugin)
	add_subdirectory(TexturePlugin)
ENDIF(Plugin_TexturePlugin)
IF(Plugin_MeshPlugin)
	add_subdirectory(MeshPlugin)
ENDIF(Plugin_MeshPlugin)

set_property(TARGET TRExplorer PROPERTY CXX_STANDARD 14)

install(
  TARGETS TRExplorer
  RUNTIME DESTINATION .
  )

install(
  FILES TRExplorer/filelist1.txt TRExplorer/filelist2.txt TRExplorer/PluginList.txt TRExplorer/NA.bmp TRExplorer/MESH.bmp
  DESTINATION .
  )

if(WIN32)
  get_filename_component(DEVIL_DLL_DIRECTORY "${IL_LIBRARIES}" DIRECTORY)
  get_filename_component(ZLIB_DLL_DIRECTORY "${ZLIB_LIBRARY_RELEASE}" DIRECTORY)
  install(
    FILES "${DEVIL_DLL_DIRECTORY}/DevIL.dll" "${DEVIL_DLL_DIRECTORY}/ILU.dll" "${DEVIL_DLL_DIRECTORY}/ILUT.dll"
    DESTINATION .
  )
  install(
    FILES "${ZLIB_DLL_DIRECTORY}/zlib1.dll"
    DESTINATION .
  )
  install(
    FILES "${wxWidgets_LIBRARY_DIRS}/wxbase30u_gcc_custom.dll"
     "${wxWidgets_LIBRARY_DIRS}/wxmsw30u_aui_gcc_custom.dll"
      "${wxWidgets_LIBRARY_DIRS}/wxmsw30u_core_gcc_custom.dll"
    DESTINATION .
  )
endif(WIN32)