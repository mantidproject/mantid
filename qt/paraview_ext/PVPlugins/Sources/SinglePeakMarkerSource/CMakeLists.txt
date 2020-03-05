project(MantidParaViewSinglePeakMarkerSource)
include_directories(SYSTEM ${PARAVIEW_INCLUDE_DIRS})
add_paraview_plugin(MantidParaViewSinglePeakMarkerSourceSMPlugin
                    "1.0"
                    SERVER_MANAGER_XML
                    SinglePeakMarkerSource.xml
                    SERVER_MANAGER_SOURCES
                    vtkSinglePeakMarkerSource.cxx)
set_pvplugin_properties(MantidParaViewSinglePeakMarkerSourceSMPlugin QT_VERSION
                        4)

target_link_libraries(MantidParaViewSinglePeakMarkerSourceSMPlugin
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${CORE_MANTIDLIBS}
                      VatesAPI)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(
    MantidParaViewSinglePeakMarkerSourceSMPlugin
    PROPERTIES
      INSTALL_RPATH
      "@loader_path/../../../Contents/Frameworks;@loader_path/../../../Contents/MacOS"
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(MantidParaViewSinglePeakMarkerSourceSMPlugin
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../../../${LIB_DIR}")
endif()

install_pvplugin(MantidParaViewSinglePeakMarkerSourceSMPlugin QT_VERSION 4)
