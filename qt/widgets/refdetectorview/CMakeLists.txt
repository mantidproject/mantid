set ( SRC_FILES
       src/RefImageDisplay.cpp
       src/RefImagePlotItem.cpp
       src/RefImageView.cpp
       src/RefSliderHandler.cpp
       src/RefRangeHandler.cpp
       src/RefIVConnections.cpp
       src/RefMatrixWSImageView.cpp
       src/RefLimitsHandler.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
       inc/MantidQtWidgets/RefDetectorView/RefImageDisplay.h
       inc/MantidQtWidgets/RefDetectorView/RefIVConnections.h
       inc/MantidQtWidgets/RefDetectorView/RefImagePlotItem.h
       inc/MantidQtWidgets/RefDetectorView/RefImageView.h
       inc/MantidQtWidgets/RefDetectorView/RefSliderHandler.h
       inc/MantidQtWidgets/RefDetectorView/RefRangeHandler.h
       inc/MantidQtWidgets/RefDetectorView/RefMatrixWSImageView.h
       inc/MantidQtWidgets/RefDetectorView/RefLimitsHandler.h
)

set ( MOC_FILES
       inc/MantidQtWidgets/RefDetectorView/RefIVConnections.h
)

set ( UI_FILES
       inc/MantidQtWidgets/RefDetectorView/RefImageView.ui
)

include_directories ( inc )
include_directories ( ../spectrumviewer/inc )

###########################################################################
# Main Library Target
###########################################################################
mtd_add_qt_library (TARGET_NAME MantidQtWidgetsRefDetectorView
                    SRC ${SRC_FILES}
                    MOC ${MOC_FILES}
                    NOMOC ${INC_FILES}
                    UI ${UI_FILES}
                    DEFS IN_MANTIDQT_REFDETECTORVIEWER
                    LINK_LIBRARIES 
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${CORE_MANTIDLIBS}
                        ${Boost_LIBRARIES}
                        ${POCO_LIBRARIES}
                        ${QT_LIBRARIES}
                        ${QWT_LIBRARIES}
                        MantidQtWidgetsSpectrumViewer
)

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtWidgetsRefDetectorViewQt4 PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

###########################################################################
# DEMO/GUI TESTING APPLICATIONS
###########################################################################
mtd_add_qt_executable (TARGET_NAME RefDetectorViewDemo
                        SRC src/RefDetectorViewDemo.cpp
                        LINK_LIBRARIES 
                            MantidQtWidgetsSpectrumViewer
                            ${TCMALLOC_LIBRARIES_LINKTIME}
                            ${Boost_LIBRARIES}
                            ${POCO_LIBRARIES}
                            ${QT_LIBRARIES}
                        MTD_QT_LINK_LIBRARIES MantidQtWidgetsRefDetectorView
                        EXCLUDE_FROM_ALL
)
