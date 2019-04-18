set(SRC_FILES
    src/CatalogPublishDialog.cpp
    src/CreateSampleShapeDialog.cpp
    src/ConvertTableToMatrixWorkspaceDialog.cpp
    src/StartLiveDataDialog.cpp
    src/FitDialog.cpp
    src/LoadDialog.cpp
    src/LoadDAEDialog.cpp
    src/LoadRawDialog.cpp
    src/LOQScriptInputDialog.cpp
    src/MantidGLWidget.cpp
    src/GetNegMuMuonicXRDDialog.cpp
    src/PeriodicTableWidget.cpp
    src/PlotAsymmetryByLogValueDialog.cpp
    src/SampleShapeHelpers.cpp
    src/SmoothNeighboursDialog.cpp
    src/SortTableWorkspaceDialog.cpp)

set(MOC_FILES
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/CatalogPublishDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/CreateSampleShapeDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/ConvertTableToMatrixWorkspaceDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/StartLiveDataDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/FitDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LoadDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LoadDAEDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LoadRawDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LOQScriptInputDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/MantidGLWidget.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/GetNegMuMuonicXRDDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/PeriodicTableWidget.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/PlotAsymmetryByLogValueDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/SampleShapeHelpers.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/SmoothNeighboursDialog.h
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/SortTableWorkspaceDialog.h)

# Include files aren't required, but this makes them appear in Visual Studio
set(INC_FILES
	${MOC_FILES})

set(UI_FILES
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/CatalogPublishDialog.ui
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/CreateSampleShapeDialog.ui
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/ConvertTableToMatrixWorkspaceDialog.ui
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/DiagScriptInputDialog.ui
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LOQScriptInputDialog.ui
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/PeriodicTableWidget.ui
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/PlotAsymmetryByLogValueDialog.ui
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LoadDialog.ui
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/StartLiveDataDialog.ui
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/FitDialog.ui
  inc/MantidQtWidgets/Plugins/AlgorithmDialogs/SortTableWorkspaceDialog.ui)

include_directories(inc inc/MantidQtWidgets/Plugins/AlgorithmDialogs)

mtd_add_qt_library(
  TARGET_NAME MantidQtWidgetsPluginsAlgorithmDialogs
  QT_VERSION 4
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  PRECOMPILED
    inc/MantidQtWidgets/Plugins/AlgorithmDialogs/PrecompiledHeader.h
  SYSTEM_INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${QT_LIBRARIES}
    ${OPENGL_LIBRARIES}
  QT4_LINK_LIBS
    Qt4::QtOpenGL
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
  OSX_INSTALL_RPATH
    @loader_path/../../Contents/MacOS
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../../${LIB_DIR}"
  INSTALL_DIR_BASE
    ${PLUGINS_DIR})

mtd_add_qt_library(
  TARGET_NAME MantidQtWidgetsPluginsAlgorithmDialogs
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  PRECOMPILED
    inc/MantidQtWidgets/Plugins/AlgorithmDialogs/PrecompiledHeader.h
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${QT_LIBRARIES}
    ${OPENGL_LIBRARIES}
  QT5_LINK_LIBS
    Qt5::OpenGL
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
  OSX_INSTALL_RPATH
    @loader_path/../../Contents/MacOS
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../../${LIB_DIR}"
  INSTALL_DIR_BASE
    ${WORKBENCH_PLUGINS_DIR})
