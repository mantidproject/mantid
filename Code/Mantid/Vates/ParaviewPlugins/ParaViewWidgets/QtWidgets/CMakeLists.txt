find_package( ParaView REQUIRED )
#bring in Paraview/VTK/QT dependencies
include(${PARAVIEW_USE_FILE})

#header files
set(INC_FILES
 WidgetDllOption.h
 GeometryWidget.h
 DimensionWidget.h
)

#header files that are mocced
set( HDR_FILES
  GeometryWidget.h
  DimensionWidget.h
)

#source files
set( SRC_FILES
  GeometryWidget.cpp
  DimensionWidget.cpp
)

#Handl QtWidget header files
qt4_wrap_cpp ( MOCCED_FILES ${HDR_FILES} )

#bring in Mantid/VATES API dependencies
set_mantid_subprojects(Vates/VatesAPI)

PROJECT(MantidParaViewQtWidgets)
ADD_LIBRARY(MantidParaViewQtWidgets ${SRC_FILES} ${MOCCED_FILES} ${INC_FILES} ) 
TARGET_LINK_LIBRARIES(MantidParaViewQtWidgets ${QT_LIBRARIES} ${MANTID_SUBPROJECT_LIBS} )

add_definitions ( -DIN_MANTIDPARAVIEWQT_MANTIDPARAVIEWWIDGETS )

