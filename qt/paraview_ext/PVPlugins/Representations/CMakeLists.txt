project(MantidParaViewRepresentation)
include_directories(SYSTEM ${PARAVIEW_INCLUDE_DIRS})
add_paraview_plugin(MantidParaViewRepresentationSMPlugin
                    "1.0"
                    SERVER_MANAGER_XML
                    Representation.xml
                    SERVER_MANAGER_SOURCES
                    vtkAlignedGeometrySliceRepresentation.cxx
                    AlignedThreeSliceFilter.cxx
                    AlignedCutter.cxx)
set_pvplugin_properties(MantidParaViewRepresentationSMPlugin QT_VERSION 4)

install_pvplugin(MantidParaViewRepresentationSMPlugin QT_VERSION 4)
