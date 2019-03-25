set(SRC_FILES
    src/RefImageDisplay.cpp
    src/RefImagePlotItem.cpp
    src/RefImageView.cpp
    src/RefSliderHandler.cpp
    src/RefRangeHandler.cpp
    src/RefIVConnections.cpp
    src/RefMatrixWSImageView.cpp
    src/RefLimitsHandler.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
set(INC_FILES
    inc/MantidQtWidgets/RefDetectorView/RefImageDisplay.h
    inc/MantidQtWidgets/RefDetectorView/RefIVConnections.h
    inc/MantidQtWidgets/RefDetectorView/RefImagePlotItem.h
    inc/MantidQtWidgets/RefDetectorView/RefImageView.h
    inc/MantidQtWidgets/RefDetectorView/RefSliderHandler.h
    inc/MantidQtWidgets/RefDetectorView/RefRangeHandler.h
    inc/MantidQtWidgets/RefDetectorView/RefMatrixWSImageView.h
    inc/MantidQtWidgets/RefDetectorView/RefLimitsHandler.h)

set(MOC_FILES
	inc/MantidQtWidgets/RefDetectorView/RefIVConnections.h)

set(UI_FILES
	inc/MantidQtWidgets/RefDetectorView/RefImageView.ui)

# Main Library Target
mtd_add_qt_library(TARGET_NAME MantidQtWidgetsRefDetectorView
                   QT_VERSION 4
                   SRC ${SRC_FILES}
                   MOC ${MOC_FILES}
                   NOMOC ${INC_FILES}
                   UI ${UI_FILES}
                   DEFS
                     IN_MANTIDQT_REFDETECTORVIEWER
                   INCLUDE_DIRS
                     inc
                   LINK_LIBS
                     ${TCMALLOC_LIBRARIES_LINKTIME}
                     ${CORE_MANTIDLIBS}
                     ${Boost_LIBRARIES}
                     ${POCO_LIBRARIES}
                   QT4_LINK_LIBS
                     Qwt5
                   MTD_QT_LINK_LIBS
                     MantidQtWidgetsSpectrumViewer
                   INSTALL_DIR
                     ${LIB_DIR}
                   OSX_INSTALL_RPATH
                     @loader_path/../MacOS
                   LINUX_INSTALL_RPATH
                     "\$ORIGIN/../${LIB_DIR}")

# DEMO/GUI TESTING APPLICATIONS
mtd_add_qt_executable(TARGET_NAME RefDetectorViewDemo
                      QT_VERSION 4
                      SRC src/RefDetectorViewDemo.cpp
                      INCLUDE_DIRS
                        ${Boost_INCLUDE_DIRS}
                      LINK_LIBS
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${Boost_LIBRARIES}
                        ${POCO_LIBRARIES}
                      QT4_LINK_LIBS
                        Qwt5
                      MTD_QT_LINK_LIBS
                        MantidQtWidgetsRefDetectorView
                        MantidQtWidgetsSpectrumViewer
                      EXCLUDE_FROM_ALL)
