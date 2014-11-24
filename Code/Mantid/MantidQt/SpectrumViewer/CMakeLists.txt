set ( SRC_FILES
       src/ColorMaps.cpp
       src/SpectrumDisplay.cpp
       src/SVUtils.cpp
       src/TrackingPicker.cpp
       src/DataArray.cpp
       src/SpectrumPlotItem.cpp
       src/QtUtils.cpp
       src/GraphDisplay.cpp
       src/SpectrumView.cpp
       src/SliderHandler.cpp
       src/RangeHandler.cpp
       src/EModeHandler.cpp
       src/SpectrumDataSource.cpp
       src/SVConnections.cpp
       src/MatrixWSDataSource.cpp
       src/MatrixWSSpectrumView.cpp
       src/ArrayDataSource.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
       inc/MantidQtSpectrumViewer/ColorMaps.h
       inc/MantidQtSpectrumViewer/SpectrumDisplay.h
       inc/MantidQtSpectrumViewer/SVConnections.h
       inc/MantidQtSpectrumViewer/DataArray.h
       inc/MantidQtSpectrumViewer/SpectrumPlotItem.h
       inc/MantidQtSpectrumViewer/SVUtils.h
       inc/MantidQtSpectrumViewer/TrackingPicker.h
       inc/MantidQtSpectrumViewer/GraphDisplay.h
       inc/MantidQtSpectrumViewer/SpectrumView.h
       inc/MantidQtSpectrumViewer/QtUtils.h
       inc/MantidQtSpectrumViewer/SpectrumDataSource.h
       inc/MantidQtSpectrumViewer/ISliderHandler.h
       inc/MantidQtSpectrumViewer/IRangeHandler.h
       inc/MantidQtSpectrumViewer/SliderHandler.h
       inc/MantidQtSpectrumViewer/RangeHandler.h
       inc/MantidQtSpectrumViewer/EModeHandler.h
       inc/MantidQtSpectrumViewer/MatrixWSDataSource.h
       inc/MantidQtSpectrumViewer/MatrixWSSpectrumView.h
       inc/MantidQtSpectrumViewer/ArrayDataSource.h
)

set ( MOC_FILES
       inc/MantidQtSpectrumViewer/SpectrumView.h
       inc/MantidQtSpectrumViewer/SVConnections.h
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
add_definitions ( -DIN_MANTIDQT_SPECTRUMVIEWER )

# This creates the target library
add_library ( MantidQtSpectrumViewer ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# ... and links to other required libs
target_link_libraries ( MantidQtSpectrumViewer
                        MantidQtAPI MantidWidgets ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES} )


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtSpectrumViewer ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


