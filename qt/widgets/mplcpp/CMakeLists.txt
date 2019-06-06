# Rules for matplotlib cpp library
set(
  LIB_SRCS
  src/Artist.cpp
  src/Axes.cpp
  src/BackendQt.cpp
  src/Colors.cpp
  src/ColorbarWidget.cpp
  src/Colormap.cpp
  src/ColorConverter.cpp
  src/Cycler.cpp
  src/ErrorbarContainer.cpp
  src/Figure.cpp
  src/FigureCanvasQt.cpp
  src/Line2D.cpp
  src/MantidAxes.cpp
  src/MantidColorMap.cpp
  src/PanZoomTool.cpp
  src/Plot.cpp
  src/ScalarMappable.cpp
  src/VerticalMarker.cpp
)

set(
  MOC_HEADERS
  inc/MantidQtWidgets/MplCpp/ColorbarWidget.h
  inc/MantidQtWidgets/MplCpp/FigureCanvasQt.h
)

set(
  NOMOC_HEADERS
  inc/MantidQtWidgets/MplCpp/Artist.h
  inc/MantidQtWidgets/MplCpp/Axes.h
  inc/MantidQtWidgets/MplCpp/Colors.h
  inc/MantidQtWidgets/MplCpp/Colormap.h
  inc/MantidQtWidgets/MplCpp/ColorConverter.h
  inc/MantidQtWidgets/MplCpp/Cycler.h
  inc/MantidQtWidgets/MplCpp/ErrorbarContainer.h
  inc/MantidQtWidgets/MplCpp/Figure.h
  inc/MantidQtWidgets/MplCpp/Line2D.h
  inc/MantidQtWidgets/MplCpp/MantidAxes.h
  inc/MantidQtWidgets/MplCpp/MantidColorMap.h
  inc/MantidQtWidgets/MplCpp/PanZoomTool.h
  inc/MantidQtWidgets/MplCpp/Plot.h
  inc/MantidQtWidgets/MplCpp/ScalarMappable.h
  inc/MantidQtWidgets/MplCpp/VerticalMarker.h
)

set(UI_FILES inc/MantidQtWidgets/MplCpp/Colorbar.ui)

find_package(BoostPython REQUIRED)

# Target
mtd_add_qt_library(
  TARGET_NAME MantidQtWidgetsMplCpp
  QT_VERSION 5
  SRC ${LIB_SRCS}
  MOC ${MOC_HEADERS}
  NOMOC ${NOMOC_HEADERS}
  UI ${UI_FILES}
  DEFS IN_MANTIDQT_MPLCPP
  INCLUDE_DIRS
    inc
    ../../../Framework/PythonInterface/core/inc
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_PATH}
    ${PYTHON_NUMPY_INCLUDE_DIR}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
    PythonInterfaceCore
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
  INSTALL_DIR
    ${LIB_DIR}
    ${WORKBENCH_LIB_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../MacOS
    @loader_path/../Libraries
  LINUX_INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}"
)

# Testing
add_subdirectory(test)
