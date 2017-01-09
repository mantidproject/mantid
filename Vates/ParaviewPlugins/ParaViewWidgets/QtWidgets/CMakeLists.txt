# header files
set( INC_FILES
 BinInputWidget.h
 WidgetDllOption.h
 LowHighStepInputWidget.h
 SimpleBinInputWidget.h
)

# header files that are mocced
set( HDR_FILES
  BinInputWidget.h
  LowHighStepInputWidget.h
  SimpleBinInputWidget.h
)

# source files
set( SRC_FILES
  LowHighStepInputWidget.cpp
  SimpleBinInputWidget.cpp
)

# Handle QtWidget header files
qt4_wrap_cpp( MOCCED_FILES ${HDR_FILES} )

project( MantidParaViewQtWidgets )
add_library( MantidParaViewQtWidgets ${SRC_FILES} ${MOCCED_FILES} ${INC_FILES} ) 
# Add to the 'Vates' group in VS
set_property( TARGET MantidParaViewQtWidgets PROPERTY FOLDER "MantidVates" )
target_link_libraries( MantidParaViewQtWidgets LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} 
vtkRenderingFreeType
${QT_LIBRARIES} 
${MANTID_SUBPROJECT_LIBS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidParaViewQtWidgets PROPERTIES INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../MacOS")
endif ()

add_definitions( -DIN_MANTIDPARAVIEWQT_MANTIDPARAVIEWWIDGETS )

install( TARGETS MantidParaViewQtWidgets ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )

