project(PeaksFilter)
include_directories(SYSTEM ${PARAVIEW_INCLUDE_DIRS})
add_paraview_plugin(MantidParaViewPeaksFilterSMPlugin
                    "1.0"
                    SERVER_MANAGER_XML
                    PeaksFilter.xml
                    SERVER_MANAGER_SOURCES
                    vtkPeaksFilter.cxx
                    vtkPeaksFilter.h)
set_pvplugin_properties(MantidParaViewPeaksFilterSMPlugin QT_VERSION 4)

target_link_libraries(MantidParaViewPeaksFilterSMPlugin
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${CORE_MANTIDLIBS}
                      DataObjects
                      VatesAPI
                      ${POCO_LIBRARIES}
                      ${Boost_LIBRARIES}
                      ${vtkjsoncpp_LIBRARIES})

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(
    MantidParaViewPeaksFilterSMPlugin
    PROPERTIES
      INSTALL_RPATH
      "@loader_path/../../../Contents/Libraries;@loader_path/../../../Contents/MacOS"
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(MantidParaViewPeaksFilterSMPlugin
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../../../${LIB_DIR}")
endif()

install_pvplugin(MantidParaViewPeaksFilterSMPlugin QT_VERSION 4)
