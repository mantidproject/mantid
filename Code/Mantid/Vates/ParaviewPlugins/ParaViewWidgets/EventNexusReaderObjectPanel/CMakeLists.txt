# Include Qt widgets
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../QtWidgets )
set_mantid_subprojects( Vates/VatesAPI )

include( ${PARAVIEW_USE_FILE} )

# So that source file shows in VS.
set( INCLUDE_FILES EventNexusReaderObjectPanel.h )
set( SRC_FILES EventNexusReaderObjectPanel.cxx )

qt4_wrap_cpp( MOC_SRCS EventNexusReaderObjectPanel.h )
add_paraview_object_panel( IFACES IFACE_SRCS 
                           CLASS_NAME EventNexusReaderObjectPanel
                           XML_NAME EventNexusReader XML_GROUP sources )
add_paraview_plugin( MantidParaViewEventNexusReaderObjectPanel "1.0"
                     GUI_INTERFACES ${IFACES}
                     SOURCES ${MOC_SRCS} ${IFACE_SRCS} ${INCLUDE_FILES} 
                     ${SRC_FILES} )
target_link_libraries( MantidParaViewEventNexusReaderObjectPanel 
MantidParaViewQtWidgets )
