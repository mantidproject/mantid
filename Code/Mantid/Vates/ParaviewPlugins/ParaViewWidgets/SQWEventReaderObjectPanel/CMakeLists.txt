# Include Qt widgets
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../QtWidgets )

# So that source file shows in VS.
set( INCLUDE_FILES SQWEventReaderObjectPanel.h )
set( SRC_FILES SQWEventReaderObjectPanel.cxx )

qt4_wrap_cpp( MOC_SRCS SQWEventReaderObjectPanel.h )
add_paraview_object_panel( IFACES IFACE_SRCS 
                           CLASS_NAME SQWEventReaderObjectPanel
                           XML_NAME DUMMY XML_GROUP sources )
add_paraview_plugin( MantidParaViewSQWEventReaderObjectPanel "1.0"
                     GUI_INTERFACES ${IFACES}
                     SOURCES ${MOC_SRCS} ${IFACE_SRCS} ${INCLUDE_FILES} 
                     ${SRC_FILES} )

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewSQWEventReaderObjectPanel PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewSQWEventReaderObjectPanel 
MantidParaViewQtWidgets )

# Put library into subfolder.
output_binary_to( MantidParaViewSQWEventReaderObjectPanel  ${PVPLUGINS_DIR})

install( TARGETS MantidParaViewSQWEventReaderObjectPanel ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR} )


