# header files
set( INC_FILES
 BinInputWidget.h
 WidgetDllOption.h
 GeometryWidget.h
 DimensionWidget.h
 DimensionWidgetFactory.h
 LowHighStepInputWidget.h
 SimpleBinInputWidget.h
 ThresholdRangeWidget.h
)

# header files that are mocced
set( HDR_FILES
  BinInputWidget.h
  GeometryWidget.h
  DimensionWidget.h
  LowHighStepInputWidget.h
  SimpleBinInputWidget.h
  ThresholdRangeWidget.h
)

# source files
set( SRC_FILES
  GeometryWidget.cpp
  DimensionWidget.cpp
  DimensionWidgetFactory.cpp
  LowHighStepInputWidget.cpp
  SimpleBinInputWidget.cpp
  ThresholdRangeWidget.cpp
)

# Handle QtWidget header files
qt4_wrap_cpp( MOCCED_FILES ${HDR_FILES} )

project( MantidParaViewQtWidgets )
add_library( MantidParaViewQtWidgets ${SRC_FILES} ${MOCCED_FILES} ${INC_FILES} ) 
# Add to the 'Vates' group in VS
set_property( TARGET MantidParaViewQtWidgets PROPERTY FOLDER "MantidVates" )
target_link_libraries( MantidParaViewQtWidgets ${QT_LIBRARIES} 
${MANTID_SUBPROJECT_LIBS} )

add_definitions( -DIN_MANTIDPARAVIEWQT_MANTIDPARAVIEWWIDGETS )

# Put library into subfolder.
#SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewQtWidgets PVPlugins)

install( TARGETS MantidParaViewQtWidgets ${TARGET_TYPE} DESTINATION ${LIB_DIR} )


