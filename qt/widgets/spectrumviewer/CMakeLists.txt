set(SRC_FILES
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
    src/ArrayDataSource.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
set(INC_FILES
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
    inc/MantidQtWidgets/SpectrumViewer/ArrayDataSource.h)

set(MOC_FILES
    inc/MantidQtWidgets/SpectrumViewer/SpectrumDisplay.h
    inc/MantidQtWidgets/SpectrumViewer/SpectrumView.h
    inc/MantidQtWidgets/SpectrumViewer/SVConnections.h
    inc/MantidQtWidgets/SpectrumViewer/TrackingPicker.h)

set(UI_FILES
	inc/MantidQtWidgets/SpectrumViewer/SpectrumView.ui)

# Main Library Target
mtd_add_qt_library(TARGET_NAME MantidQtWidgetsSpectrumViewer
                   QT_VERSION 4
                   SRC ${SRC_FILES}
                   MOC ${MOC_FILES}
                   NOMOC ${INC_FILES}
                   UI ${UI_FILES}
                   DEFS IN_MANTIDQT_SPECTRUMVIEWER
                   INCLUDE_DIRS
                     inc
                   SYSTEM_INCLUDE_DIRS
                     ${Boost_INCLUDE_DIRS}
                   LINK_LIBS
                     ${TCMALLOC_LIBRARIES_LINKTIME}
                     ${CORE_MANTIDLIBS}
                     ${POCO_LIBRARIES}
                     ${Boost_LIBRARIES}
                   QT4_LINK_LIBS
                     Qwt5
                   MTD_QT_LINK_LIBS
                     MantidQtWidgetsCommon
                     MantidQtWidgetsPlotting
                   INSTALL_DIR
                     ${LIB_DIR}
                   OSX_INSTALL_RPATH
                     @loader_path/../MacOS
                     @loader_path/../Libraries
                   LINUX_INSTALL_RPATH
                     "\$ORIGIN/../${LIB_DIR}")
