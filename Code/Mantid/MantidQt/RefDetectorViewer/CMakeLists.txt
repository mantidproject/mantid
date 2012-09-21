set ( SRC_FILES
       src/ColorMaps.cpp        
       src/RefImageDisplay.cpp   
       src/IVUtils.cpp        
       src/TrackingPicker.cpp
       src/DataArray.cpp        
       src/RefImagePlotItem.cpp  
       src/QtUtils.cpp
       src/GraphDisplay.cpp     
       src/ImageView.cpp      
       src/SliderHandler.cpp
       src/RangeHandler.cpp
       src/ImageDataSource.cpp  
       src/RefIVConnections.cpp 
       src/MatrixWSDataSource.cpp
       src/MatrixWSImageView.cpp
       src/ArrayDataSource.cpp
       src/ErrorHandler.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
       inc/MantidQtRefDetectorViewer/ColorMaps.h
       inc/MantidQtRefDetectorViewer/RefImageDisplay.h
       inc/MantidQtRefDetectorViewer/RefIVConnections.h
       inc/MantidQtRefDetectorViewer/DataArray.h
       inc/MantidQtRefDetectorViewer/RefImagePlotItem.h
       inc/MantidQtRefDetectorViewer/IVUtils.h
       inc/MantidQtRefDetectorViewer/TrackingPicker.h
       inc/MantidQtRefDetectorViewer/GraphDisplay.h
       inc/MantidQtRefDetectorViewer/ImageView.h
       inc/MantidQtRefDetectorViewer/QtUtils.h
       inc/MantidQtRefDetectorViewer/ImageDataSource.h
       inc/MantidQtRefDetectorViewer/SliderHandler.h
       inc/MantidQtRefDetectorViewer/RangeHandler.h
       inc/MantidQtRefDetectorViewer/MatrixWSDataSource.h
       inc/MantidQtRefDetectorViewer/MatrixWSImageView.h
       inc/MantidQtRefDetectorViewer/ArrayDataSource.h
       inc/MantidQtRefDetectorViewer/ErrorHandler.h
)

set ( MOC_FILES 
       inc/MantidQtRefDetectorViewer/RefIVConnections.h
       inc/MantidQtRefDetectorViewer/TrackingPicker.h
)

set ( UI_FILES 
       inc/MantidQtRefDetectorViewer/ImageView.ui
)

# Python unit tests
set ( TEST_PY_FILES 
)

include_directories ( inc )
include_directories ( ../../Framework/API/inc )


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
target_link_libraries ( RefDetectorViewDemo MantidQtAPI QtPropertyBrowser
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


