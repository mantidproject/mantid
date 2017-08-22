set ( SRC_FILES
	src/WidgetFactory.cpp
)

set ( INC_FILES
	inc/MantidQtWidgets/Factory/WidgetFactory.h
)

set ( MOC_FILES 
	inc/MantidQtWidgets/Factory/WidgetFactory.h
)

set ( UI_FILES 
)

set ( TEST_FILES
	test/WidgetFactoryTest.h
)

include_directories ( inc )
include_directories ( ../sliceviewer/inc )

###########################################################################
# Main Library Target
###########################################################################

#qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )
#set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )
#qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../sliceviewer )

# For Windows:
add_definitions ( -DIN_MANTIDQT_FACTORY )

# Target
mtd_add_qt_library (TARGET_NAME MantidQtWidgetsFactory
                    SRC ${SRC_FILES}
                    MOC ${MOC_FILES}
                    NOMOC ${INC_FILES}
                    #INCLUDE_DIRS ${INCLUDES}
                    LINK_LIBRARIES  MantidQtWidgetsCommon
                                    MantidQtWidgetsSliceViewer
                                    ${CORE_MANTIDLIBS}
                                    ${POCO_LIBRARIES}
                                    ${Boost_LIBRARIES}
                                    ${QT_LIBRARIES}
                                    ${QWT_LIBRARIES}
)

# This creates the target library
#add_library ( MantidQtWidgetsFactory ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtWidgetsFactory PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

# ... and links to other required libs
#target_link_libraries ( MantidQtWidgetsFactory LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} 
#            MantidQtWidgetsCommon
#            MantidQtWidgetsSliceViewer
#            ${CORE_MANTIDLIBS}
#            ${POCO_LIBRARIES}
#            ${Boost_LIBRARIES}
#            ${QT_LIBRARIES}
#            ${QWT_LIBRARIES} )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtWidgetsFactoryQt4 ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


