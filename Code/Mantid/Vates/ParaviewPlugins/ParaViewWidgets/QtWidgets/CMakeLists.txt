# header files
set( INC_FILES
 WidgetDllOption.h
 GeometryWidget.h
 DimensionWidget.h
 DimensionWidgetFactory.h
 ThresholdRangeWidget.h
)

# header files that are mocced
set( HDR_FILES
  GeometryWidget.h
  DimensionWidget.h
  DimensionWidgetFactory.h
  ThresholdRangeWidget.h
)

# source files
set( SRC_FILES
  GeometryWidget.cpp
  DimensionWidget.cpp
  DimensionWidgetFactory.cpp
  ThresholdRangeWidget.cpp
)

# Handle QtWidget header files
qt4_wrap_cpp( MOCCED_FILES ${HDR_FILES} )

project( MantidParaViewQtWidgets )
add_library( MantidParaViewQtWidgets ${SRC_FILES} ${MOCCED_FILES} ${INC_FILES} ) 
target_link_libraries( MantidParaViewQtWidgets ${QT_LIBRARIES} 
${MANTID_SUBPROJECT_LIBS} )

add_definitions( -DIN_MANTIDPARAVIEWQT_MANTIDPARAVIEWWIDGETS )

# Put library into subfolder.
#output_binary_to(MantidParaViewQtWidgets PVPlugins)

install( TARGETS MantidParaViewQtWidgets ${TARGET_TYPE} DESTINATION ${LIB_DIR} )


