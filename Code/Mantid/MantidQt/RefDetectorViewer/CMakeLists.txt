set ( SRC_FILES
       src/RefImageDisplay.cpp   
       src/RefImagePlotItem.cpp  
       src/RefImageView.cpp      
       src/SliderHandler.cpp
       src/RangeHandler.cpp
       src/RefIVConnections.cpp 
       src/RefMatrixWSDataSource.cpp
       src/RefMatrixWSImageView.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
       inc/MantidQtRefDetectorViewer/RefImageDisplay.h
       inc/MantidQtRefDetectorViewer/RefIVConnections.h
       inc/MantidQtRefDetectorViewer/RefImagePlotItem.h
       inc/MantidQtRefDetectorViewer/RefImageView.h
       inc/MantidQtRefDetectorViewer/SliderHandler.h
       inc/MantidQtRefDetectorViewer/RangeHandler.h
       inc/MantidQtRefDetectorViewer/RefMatrixWSDataSource.h
       inc/MantidQtRefDetectorViewer/RefMatrixWSImageView.h
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
include_directories ( ../ImageViewer/inc )


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
add_definitions ( -DIN_MANTIDQT_IMAGEVIEWER )

# This creates the target library
add_library ( MantidQtRefDetectorViewer ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# ... and links to other required libs
target_link_libraries ( MantidQtRefDetectorViewer
                        MantidQtAPI MantidWidgets QtPropertyBrowser ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# DEMO/GUI TESTING APPLICATIONS
###########################################################################

add_executable ( RefDetectorViewDemo EXCLUDE_FROM_ALL ${ALL_SRC} src/RefDetectorViewDemo.cpp ${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS})
target_link_libraries ( RefDetectorViewDemo MantidQtAPI QtPropertyBrowser MantidQtImageViewer
                        ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES} )

# add_executable ( ImageViewNxEventFile EXCLUDE_FROM_ALL ${ALL_SRC}
# src/ImageViewNxEventFile.cpp ${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS})
# target_link_libraries ( ImageViewNxEventFile 
                       # MantidQtAPI QtPropertyBrowser ${CORE_MANTIDLIBS}
# ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# Unit tests setup
###########################################################################


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtRefDetectorViewer ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


