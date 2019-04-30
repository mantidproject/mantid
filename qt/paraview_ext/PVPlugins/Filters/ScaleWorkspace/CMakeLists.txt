project(ScaleWorkspace)
include_directories(SYSTEM ${PARAVIEW_INCLUDE_DIRS})
add_paraview_plugin(MantidParaViewScaleWorkspaceSMPlugin
                    "1.0"
                    SERVER_MANAGER_XML
                    ScaleWorkspace.xml
                    SERVER_MANAGER_SOURCES
                    vtkScaleWorkspace.cxx)
set_pvplugin_properties(MantidParaViewScaleWorkspaceSMPlugin QT_VERSION 4)

target_link_libraries(MantidParaViewScaleWorkspaceSMPlugin
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${CORE_MANTIDLIBS}
                      DataObjects
                      VatesAPI
                      ${vtkjsoncpp_LIBRARIES})

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(
    MantidParaViewScaleWorkspaceSMPlugin
    PROPERTIES
      INSTALL_RPATH
      "@loader_path/../../../Contents/Libraries;@loader_path/../../../Contents/MacOS"
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(MantidParaViewScaleWorkspaceSMPlugin
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../../../${LIB_DIR}")
endif()

install_pvplugin(MantidParaViewScaleWorkspaceSMPlugin QT_VERSION 4)
