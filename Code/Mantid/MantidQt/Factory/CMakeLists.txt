set ( SRC_FILES
	src/WidgetFactory.cpp
)

set ( INC_FILES
	inc/MantidQtFactory/WidgetFactory.h
)

set ( MOC_FILES 
	inc/MantidQtFactory/WidgetFactory.h
)

set ( UI_FILES 
)

set ( TEST_FILES
	test/WidgetFactoryTest.h
)

include_directories ( inc )
include_directories ( ../SliceViewer/inc )

###########################################################################
# Main Library Target
###########################################################################

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 

include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../SliceViewer )

# For Windows:
add_definitions ( -DIN_MANTIDQT_FACTORY )

# This creates the target library
add_library ( MantidQtFactory ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

set_target_properties ( MantidQtFactory PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")

# ... and links to other required libs
target_link_libraries ( MantidQtFactory MantidQtAPI MantidQtSliceViewer ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtFactory ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


