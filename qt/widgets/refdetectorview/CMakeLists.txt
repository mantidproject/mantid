set ( SRC_FILES
       src/RefImageDisplay.cpp
       src/RefImagePlotItem.cpp
       src/RefImageView.cpp
       src/RefSliderHandler.cpp
       src/RefRangeHandler.cpp
       src/RefIVConnections.cpp
       src/RefMatrixWSImageView.cpp
       src/RefLimitsHandler.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
       inc/MantidQtWidgets/RefDetectorView/RefImageDisplay.h
       inc/MantidQtWidgets/RefDetectorView/RefIVConnections.h
       inc/MantidQtWidgets/RefDetectorView/RefImagePlotItem.h
       inc/MantidQtWidgets/RefDetectorView/RefImageView.h
       inc/MantidQtWidgets/RefDetectorView/RefSliderHandler.h
       inc/MantidQtWidgets/RefDetectorView/RefRangeHandler.h
       inc/MantidQtWidgets/RefDetectorView/RefMatrixWSImageView.h
       inc/MantidQtWidgets/RefDetectorView/RefLimitsHandler.h
)

set ( MOC_FILES
       inc/MantidQtWidgets/RefDetectorView/RefIVConnections.h
)

set ( UI_FILES
       inc/MantidQtWidgets/RefDetectorView/RefImageView.ui
)

include_directories ( inc )
include_directories ( ../spectrumviewer/inc )


###########################################################################
# Main Library Target
###########################################################################

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

# --- Add icons qrc ------

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )

include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories ( inc )

# For Windows:
add_definitions ( -DIN_MANTIDQT_REFDETECTORVIEWER )

# This creates the target library
add_library ( MantidQtWidgetsRefDetectorView ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtWidgetsRefDetectorView PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

# ... and links to other required libs
target_link_libraries ( MantidQtWidgetsRefDetectorView LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsSpectrumViewer
            ${CORE_MANTIDLIBS}
            ${Boost_LIBRARIES}
            ${POCO_LIBRARIES}
            ${QT_LIBRARIES}
            ${QWT_LIBRARIES}
            )

###########################################################################
# DEMO/GUI TESTING APPLICATIONS
###########################################################################

add_executable ( RefDetectorViewDemo EXCLUDE_FROM_ALL src/RefDetectorViewDemo.cpp )
target_link_libraries ( RefDetectorViewDemo LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsRefDetectorView
            MantidQtWidgetsSpectrumViewer
            ${Boost_LIBRARIES}
            ${POCO_LIBRARIES}
            ${QT_LIBRARIES}
            )

###########################################################################
# Unit tests setup
###########################################################################


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtWidgetsRefDetectorView ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
