project(MantidParaViewPeaksReader)
include_directories(SYSTEM ${PARAVIEW_INCLUDE_DIRS})
add_paraview_plugin(MantidParaViewPeaksReaderSMPlugin
                    "1.0"
                    SERVER_MANAGER_XML
                    PeaksReader.xml
                    SERVER_MANAGER_SOURCES
                    vtkPeaksReader.cxx)
set_pvplugin_properties(MantidParaViewPeaksReaderSMPlugin QT_VERSION 4)

# Add to the 'VatesParaViewPlugins' group in VS
set_property(TARGET MantidParaViewPeaksReaderSMPlugin
             PROPERTY FOLDER "MantidVatesParaViewPlugins")

target_link_libraries(MantidParaViewPeaksReaderSMPlugin
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${CORE_MANTIDLIBS}
                      DataObjects
                      VatesAPI
                      ${POCO_LIBRARIES}
                      ${Boost_LIBRARIES})

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(
    MantidParaViewPeaksReaderSMPlugin
    PROPERTIES
      INSTALL_RPATH
      "@loader_path/../../../Contents/Libraries;@loader_path/../../../Contents/MacOS"
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(MantidParaViewPeaksReaderSMPlugin
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../../../${LIB_DIR}")
endif()

install_pvplugin(MantidParaViewPeaksReaderSMPlugin QT_VERSION 4)
