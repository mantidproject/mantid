project(SplatterPlot)
include_directories(SYSTEM ${PARAVIEW_INCLUDE_DIRS})
add_paraview_plugin(MantidParaViewSplatterPlotSMPlugin
                    "1.0"
                    SERVER_MANAGER_XML
                    SplatterPlot.xml
                    SERVER_MANAGER_SOURCES
                    vtkSplatterPlot.cxx
                    GUI_RESOURCES
                    SplatterPlot.qrc)
set_pvplugin_properties(MantidParaViewSplatterPlotSMPlugin QT_VERSION 4)

include_directories(SYSTEM ${QWT5_INCLUDE_DIR})

target_link_libraries(MantidParaViewSplatterPlotSMPlugin
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${CORE_MANTIDLIBS}
                      DataObjects
                      VatesAPI
                      ${POCO_LIBRARIES}
                      ${Boost_LIBRARIES}
                      ${QWT5_LIBRARIES}
                      ${vtkjsoncpp_LIBRARIES})

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(
    MantidParaViewSplatterPlotSMPlugin
    PROPERTIES
      INSTALL_RPATH
      "@loader_path/../../../Contents/Libraries;@loader_path/../../../Contents/MacOS"
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(MantidParaViewSplatterPlotSMPlugin
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../../../${LIB_DIR}")
endif()

install_pvplugin(MantidParaViewSplatterPlotSMPlugin QT_VERSION 4)
