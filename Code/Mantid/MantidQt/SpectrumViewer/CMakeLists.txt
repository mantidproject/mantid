set ( SRC_FILES
       src/ColorMaps.cpp        
       src/ImageDisplay.cpp   
       src/IVUtils.cpp        
       src/TrackingPicker.cpp
       src/DataArray.cpp        
       src/SpectrumPlotItem.cpp  
       src/QtUtils.cpp
       src/GraphDisplay.cpp     
       src/SpectrumView.cpp      
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
       inc/MantidQtSpectrumViewer/ColorMaps.h
       inc/MantidQtSpectrumViewer/ImageDisplay.h
       inc/MantidQtSpectrumViewer/IVConnections.h
       inc/MantidQtSpectrumViewer/DataArray.h
       inc/MantidQtSpectrumViewer/SpectrumPlotItem.h
       inc/MantidQtSpectrumViewer/IVUtils.h
       inc/MantidQtSpectrumViewer/TrackingPicker.h
       inc/MantidQtSpectrumViewer/GraphDisplay.h
       inc/MantidQtSpectrumViewer/SpectrumView.h
       inc/MantidQtSpectrumViewer/QtUtils.h
       inc/MantidQtSpectrumViewer/ImageDataSource.h
       inc/MantidQtSpectrumViewer/ISliderHandler.h
       inc/MantidQtSpectrumViewer/IRangeHandler.h
       inc/MantidQtSpectrumViewer/SliderHandler.h
       inc/MantidQtSpectrumViewer/RangeHandler.h
       inc/MantidQtSpectrumViewer/EModeHandler.h
       inc/MantidQtSpectrumViewer/MatrixWSDataSource.h
       inc/MantidQtSpectrumViewer/MatrixWSImageView.h
       inc/MantidQtSpectrumViewer/ArrayDataSource.h
       inc/MantidQtSpectrumViewer/ErrorHandler.h
)

set ( MOC_FILES 
       inc/MantidQtSpectrumViewer/IVConnections.h
       inc/MantidQtSpectrumViewer/TrackingPicker.h
)

set ( UI_FILES 
       inc/MantidQtSpectrumViewer/SpectrumView.ui
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
add_library ( MantidQtSpectrumViewer ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# ... and links to other required libs
target_link_libraries ( MantidQtSpectrumViewer
                        MantidQtAPI MantidWidgets ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# DEMO/GUI TESTING APPLICATIONS
###########################################################################

if ( NOT WIN32 )
add_executable ( ImageViewDemo EXCLUDE_FROM_ALL src/ImageViewDemo.cpp )
target_link_libraries ( ImageViewDemo MantidQtSpectrumViewer )

add_executable ( ImageViewNxEventFile EXCLUDE_FROM_ALL src/ImageViewNxEventFile.cpp )
target_link_libraries ( ImageViewNxEventFile MantidQtSpectrumViewer )
endif ()

###########################################################################
# Unit tests setup
###########################################################################


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtSpectrumViewer ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


