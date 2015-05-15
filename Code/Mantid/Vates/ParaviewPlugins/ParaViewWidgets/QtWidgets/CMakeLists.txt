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
target_link_libraries( MantidParaViewQtWidgets 
vtkRenderingFreeType
vtkRenderingFreeTypeOpenGL
${QT_LIBRARIES} 
${MANTID_SUBPROJECT_LIBS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidParaViewQtWidgets PROPERTIES INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../MacOS")
endif ()

add_definitions( -DIN_MANTIDPARAVIEWQT_MANTIDPARAVIEWWIDGETS )

# Put library into subfolder.
#SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewQtWidgets PVPlugins)

install( TARGETS MantidParaViewQtWidgets ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )

