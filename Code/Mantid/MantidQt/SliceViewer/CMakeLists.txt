set ( SRC_FILES 
	src/CustomTools.cpp
	src/DimensionSliceWidget.cpp
	src/QwtRasterDataMD.cpp
	src/SliceViewer.cpp
	src/ColorBarWidget.cpp
	src/QScienceSpinBox.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	inc/MantidQtSliceViewer/CustomTools.h
	inc/MantidQtSliceViewer/DimensionSliceWidget.h
	inc/MantidQtSliceViewer/QwtRasterDataMD.h
	inc/MantidQtSliceViewer/SliceViewer.h
	inc/MantidQtSliceViewer/ColorBarWidget.h
	inc/MantidQtSliceViewer/QScienceSpinBox.h
)

set ( MOC_FILES 
	inc/MantidQtSliceViewer/CustomTools.h
	inc/MantidQtSliceViewer/DimensionSliceWidget.h
	inc/MantidQtSliceViewer/SliceViewer.h
	inc/MantidQtSliceViewer/ColorBarWidget.h
	inc/MantidQtSliceViewer/QScienceSpinBox.h
)

set ( UI_FILES 
	inc/MantidQtSliceViewer/DimensionSliceWidget.ui
	inc/MantidQtSliceViewer/SliceViewer.ui
	inc/MantidQtSliceViewer/ColorBarWidget.ui
)


include_directories ( inc )
include_directories ( ../../QtPropertyBrowser/src )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )


set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# For Windows:
add_definitions ( -DIN_MANTIDQT_SLICEVIEWER )
add_library ( MantidQtSliceViewer ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

target_link_libraries ( MantidQtSliceViewer MantidQtAPI QtPropertyBrowser ${QT_LIBRARIES} ${QWT_LIBRARIES} )

# Extra, optional target for demoing a GUI element
add_executable ( ColorBarWidgetDemo EXCLUDE_FROM_ALL ${ALL_SRC} src/main_ColorBarWidget.cpp ${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS})
target_link_libraries ( ColorBarWidgetDemo MantidQtAPI QtPropertyBrowser ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtSliceViewer DESTINATION ${LIB_DIR} )
