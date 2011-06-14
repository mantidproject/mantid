# header files
set( INC_FILES
 WidgetDllOption.h
 GeometryWidget.h
 DimensionWidget.h
 DimensionWidgetFactory.h
)

# header files that are mocced
set( HDR_FILES
  GeometryWidget.h
  DimensionWidget.h
  DimensionWidgetFactory.h
)

# source files
set( SRC_FILES
  GeometryWidget.cpp
  DimensionWidget.cpp
  DimensionWidgetFactory.cpp
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

# Windows-only at the moment.
if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
  install(TARGETS MantidParaViewQtWidgets RUNTIME DESTINATION ${LIB_DIR}/PVPlugins)
endif( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )

