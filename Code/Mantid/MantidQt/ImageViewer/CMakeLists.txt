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
       src/ImageDataSource.cpp  
       src/IVConnections.cpp 
       src/TestDataSource.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
       inc/MantidQtImageViewer/ColorMaps.h
       inc/MantidQtImageViewer/ImageDisplay.h
       inc/MantidQtImageViewer/IVConnections.h
       inc/MantidQtImageViewer/TestDataSource.h
       inc/MantidQtImageViewer/DataArray.h
       inc/MantidQtImageViewer/ImagePlotItem.h
       inc/MantidQtImageViewer/IVUtils.h
       inc/MantidQtImageViewer/TrackingPicker.h
       inc/MantidQtImageViewer/GraphDisplay.h
       inc/MantidQtImageViewer/ImageView.h
       inc/MantidQtImageViewer/QtUtils.h
       inc/MantidQtImageViewer/ImageDataSource.h
       inc/MantidQtImageViewer/ImageView.ui
       inc/MantidQtImageViewer/SliderHandler.h
       inc/MantidQtImageViewer/TestDataSource.h
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
add_library ( MantidQtImageViewer ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# ... and links to other required libs
target_link_libraries ( MantidQtImageViewer MantidQtAPI MantidWidgets QtPropertyBrowser ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# DEMO/GUI TESTING APPLICATIONS
###########################################################################

if ( WIN32 )
# Does not work on windows for some reason
else ()

add_executable ( ImageViewDemo EXCLUDE_FROM_ALL ${ALL_SRC} src/ImageViewDemo.cpp ${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS})
target_link_libraries ( ImageViewDemo MantidQtAPI QtPropertyBrowser ${QT_LIBRARIES} ${QWT_LIBRARIES} )

endif ()

###########################################################################
# Unit tests setup
###########################################################################


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtImageViewer DESTINATION ${LIB_DIR} )


