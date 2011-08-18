# Include Qt widgets
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../QtWidgets )

# So that source file shows in VS.
set( INCLUDE_FILES MDEWNexusReaderObjectPanel.h )
set( SRC_FILES MDEWNexusReaderObjectPanel.cxx )

qt4_wrap_cpp( MOC_SRCS MDEWNexusReaderObjectPanel.h )
add_paraview_object_panel( IFACES IFACE_SRCS 
                           CLASS_NAME MDEWNexusReaderObjectPanel
                           XML_NAME DUMMY XML_GROUP sources )
add_paraview_plugin( MantidParaViewMDEWNexusReaderObjectPanel "1.0"
                     GUI_INTERFACES ${IFACES}
                     SOURCES ${MOC_SRCS} ${IFACE_SRCS} ${INCLUDE_FILES} 
                     ${SRC_FILES} )

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewMDEWNexusReaderObjectPanel PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewMDEWNexusReaderObjectPanel 
MantidParaViewQtWidgets )

# Put library into subfolder.
output_binary_to( MantidParaViewMDEWNexusReaderObjectPanel  ${PVPLUGINS_DIR})

install( TARGETS MantidParaViewMDEWNexusReaderObjectPanel ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR} )


