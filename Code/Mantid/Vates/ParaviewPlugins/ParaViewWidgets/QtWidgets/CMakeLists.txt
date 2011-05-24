# header files
set( INC_FILES
 WidgetDllOption.h
 GeometryWidget.h
 DimensionWidget.h
)

# header files that are mocced
set( HDR_FILES
  GeometryWidget.h
  DimensionWidget.h
)

# source files
set( SRC_FILES
  GeometryWidget.cpp
  DimensionWidget.cpp
)

# Handle QtWidget header files
qt4_wrap_cpp( MOCCED_FILES ${HDR_FILES} )

project( MantidParaViewQtWidgets )
add_library( MantidParaViewQtWidgets ${SRC_FILES} ${MOCCED_FILES} ${INC_FILES} ) 
target_link_libraries( MantidParaViewQtWidgets ${QT_LIBRARIES} 
${MANTID_SUBPROJECT_LIBS} )

add_definitions( -DIN_MANTIDPARAVIEWQT_MANTIDPARAVIEWWIDGETS )

