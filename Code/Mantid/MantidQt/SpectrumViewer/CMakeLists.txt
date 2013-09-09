set ( SRC_FILES
       src/ColorMaps.cpp        
       src/ImageDisplay.cpp   
       src/IVUtils.cpp        
       src/TrackingPicker.cpp
       src/DataArray.cpp        
       src/ImagePlotItem.cpp  
       src/QtUtils.cpp
       src/GraphDisplay.cpp     
       src/ImageView.cpp      
       src/SliderHandler.cpp
       src/RangeHandler.cpp
       src/EModeHandler.cpp
       src/ImageDataSource.cpp  
       src/IVConnections.cpp 
       src/MatrixWSDataSource.cpp
       src/MatrixWSImageView.cpp
       src/ArrayDataSource.cpp
       src/ErrorHandler.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
       inc/MantidQtImageViewer/ColorMaps.h
       inc/MantidQtImageViewer/ImageDisplay.h
       inc/MantidQtImageViewer/IVConnections.h
       inc/MantidQtImageViewer/DataArray.h
       inc/MantidQtImageViewer/ImagePlotItem.h
       inc/MantidQtImageViewer/IVUtils.h
       inc/MantidQtImageViewer/TrackingPicker.h
       inc/MantidQtImageViewer/GraphDisplay.h
       inc/MantidQtImageViewer/ImageView.h
       inc/MantidQtImageViewer/QtUtils.h
       inc/MantidQtImageViewer/ImageDataSource.h
       inc/MantidQtImageViewer/ISliderHandler.h
       inc/MantidQtImageViewer/IRangeHandler.h
       inc/MantidQtImageViewer/SliderHandler.h
       inc/MantidQtImageViewer/RangeHandler.h
       inc/MantidQtImageViewer/EModeHandler.h
       inc/MantidQtImageViewer/MatrixWSDataSource.h
       inc/MantidQtImageViewer/MatrixWSImageView.h
       inc/MantidQtImageViewer/ArrayDataSource.h
       inc/MantidQtImageViewer/ErrorHandler.h
)

set ( MOC_FILES 
       inc/MantidQtImageViewer/IVConnections.h
       inc/MantidQtImageViewer/TrackingPicker.h
)

set ( UI_FILES 
       inc/MantidQtImageViewer/ImageView.ui
)

# Python unit tests
set ( TEST_PY_FILES 
)

include_directories ( inc )


###########################################################################
# Main Library Target
###########################################################################

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

# --- Add icons qrc ------

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 

include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# For Windows:
add_definitions ( -DIN_MANTIDQT_IMAGEVIEWER )

# This creates the target library
add_library ( MantidQtImageViewer ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# ... and links to other required libs
target_link_libraries ( MantidQtImageViewer
                        MantidQtAPI MantidWidgets ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# DEMO/GUI TESTING APPLICATIONS
###########################################################################

if ( NOT WIN32 )
add_executable ( ImageViewDemo EXCLUDE_FROM_ALL src/ImageViewDemo.cpp )
target_link_libraries ( ImageViewDemo MantidQtImageViewer )

add_executable ( ImageViewNxEventFile EXCLUDE_FROM_ALL src/ImageViewNxEventFile.cpp )
target_link_libraries ( ImageViewNxEventFile MantidQtImageViewer )
endif ()

###########################################################################
# Unit tests setup
###########################################################################


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtImageViewer ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


