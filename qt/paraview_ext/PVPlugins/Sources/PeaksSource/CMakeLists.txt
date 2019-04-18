project(MantidParaViewPeaksSource)
include_directories(SYSTEM ${PARAVIEW_INCLUDE_DIRS})
add_paraview_plugin(MantidParaViewPeaksSourceSMPlugin
                    "1.0"
                    SERVER_MANAGER_XML
                    PeaksSource.xml
                    SERVER_MANAGER_SOURCES
                    vtkPeaksSource.cxx)
set_pvplugin_properties(MantidParaViewPeaksSourceSMPlugin QT_VERSION 4)

target_link_libraries(MantidParaViewPeaksSourceSMPlugin
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${CORE_MANTIDLIBS}
                      DataObjects
                      VatesAPI
                      ${POCO_LIBRARIES}
                      ${Boost_LIBRARIES})

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(
    MantidParaViewPeaksSourceSMPlugin
    PROPERTIES
      INSTALL_RPATH
      "@loader_path/../../../Contents/Libraries;@loader_path/../../../Contents/MacOS"
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(MantidParaViewPeaksSourceSMPlugin
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../../../${LIB_DIR}")
endif()

install_pvplugin(MantidParaViewPeaksSourceSMPlugin QT_VERSION 4)
