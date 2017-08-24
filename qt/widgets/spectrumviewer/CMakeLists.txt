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
       #src/MatrixWSSpectrumView.cpp
       src/ArrayDataSource.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
       inc/MantidQtWidgets/SpectrumViewer/ColorMaps.h
       inc/MantidQtWidgets/SpectrumViewer/SpectrumDisplay.h
       inc/MantidQtWidgets/SpectrumViewer/SVConnections.h
       inc/MantidQtWidgets/SpectrumViewer/DataArray.h
       inc/MantidQtWidgets/SpectrumViewer/SpectrumPlotItem.h
       inc/MantidQtWidgets/SpectrumViewer/SVUtils.h
       inc/MantidQtWidgets/SpectrumViewer/TrackingPicker.h
       inc/MantidQtWidgets/SpectrumViewer/GraphDisplay.h
       inc/MantidQtWidgets/SpectrumViewer/SpectrumView.h
       inc/MantidQtWidgets/SpectrumViewer/QtUtils.h
       inc/MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h
       inc/MantidQtWidgets/SpectrumViewer/ISliderHandler.h
       inc/MantidQtWidgets/SpectrumViewer/IRangeHandler.h
       inc/MantidQtWidgets/SpectrumViewer/SliderHandler.h
       inc/MantidQtWidgets/SpectrumViewer/RangeHandler.h
       inc/MantidQtWidgets/SpectrumViewer/EModeHandler.h
       inc/MantidQtWidgets/SpectrumViewer/MatrixWSDataSource.h
      #inc/MantidQtWidgets/SpectrumViewer/MatrixWSSpectrumView.h
       inc/MantidQtWidgets/SpectrumViewer/ArrayDataSource.h
)

set ( MOC_FILES
       inc/MantidQtWidgets/SpectrumViewer/SpectrumDisplay.h
       inc/MantidQtWidgets/SpectrumViewer/SpectrumView.h
       inc/MantidQtWidgets/SpectrumViewer/SVConnections.h
       inc/MantidQtWidgets/SpectrumViewer/TrackingPicker.h
)

set ( UI_FILES
       inc/MantidQtWidgets/SpectrumViewer/SpectrumView.ui
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
add_library ( MantidQtWidgetsSpectrumViewer ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtWidgetsSpectrumViewer PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()
# ... and links to other required libs
target_link_libraries ( MantidQtWidgetsSpectrumViewer LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsCommonQt4
            ${CORE_MANTIDLIBS}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${QT_LIBRARIES}
            ${QWT_LIBRARIES} )


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtWidgetsSpectrumViewer ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
