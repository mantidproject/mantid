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
       inc/MantidQtRefDetectorViewer/RefImageDisplay.h
       inc/MantidQtRefDetectorViewer/RefIVConnections.h
       inc/MantidQtRefDetectorViewer/RefImagePlotItem.h
       inc/MantidQtRefDetectorViewer/RefImageView.h
       inc/MantidQtRefDetectorViewer/RefSliderHandler.h
       inc/MantidQtRefDetectorViewer/RefRangeHandler.h
       inc/MantidQtRefDetectorViewer/RefMatrixWSImageView.h
       inc/MantidQtRefDetectorViewer/RefLimitsHandler.h
)

set ( MOC_FILES 
       inc/MantidQtRefDetectorViewer/RefIVConnections.h
)

set ( UI_FILES 
       inc/MantidQtRefDetectorViewer/RefImageView.ui
)

# Python unit tests
set ( TEST_PY_FILES 
)

include_directories ( inc )
include_directories ( ../SpectrumViewer/inc )


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
add_library ( MantidQtRefDetectorViewer ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtRefDetectorViewer PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

# ... and links to other required libs
target_link_libraries ( MantidQtRefDetectorViewer MantidQtSpectrumViewer )

###########################################################################
# DEMO/GUI TESTING APPLICATIONS
###########################################################################

add_executable ( RefDetectorViewDemo EXCLUDE_FROM_ALL src/RefDetectorViewDemo.cpp )
target_link_libraries ( RefDetectorViewDemo MantidQtRefDetectorViewer )

###########################################################################
# Unit tests setup
###########################################################################


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtRefDetectorViewer ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


