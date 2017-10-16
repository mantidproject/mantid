# Require additional Qt packages
find_package ( Qt4 COMPONENTS QtDesigner REQUIRED )
include ( ${QT_USE_FILE} )

set ( SRC_FILES 
    src/DesignerPlugin.cpp
    src/PluginCollectionInterface.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES 
    inc/MantidQtWidgets/Plugins/Designer/DesignerPlugin.h
    inc/MantidQtWidgets/Plugins/Designer/PluginCollectionInterface.h
)

set ( MOC_FILES ${INC_FILES} )

include_directories ( inc )
include_directories ( ../../sliceviewer/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../../sliceviewer )
include_directories ( ${QSCINTILLA_INCLUDE_DIR} )

mtd_add_qt_library (TARGET_NAME MantidQtWidgetsPluginsDesigner
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  DEFS QSCINTILLA_DLL
  QT_PLUGIN MantidWidgetPlugins
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
  QT4_LINK_LIBS
    Qt4::Qscintilla
    Qwt5
  MTD_QT_LINK_LIBS
      MantidQtWidgetsCommon
      MantidQtWidgetsSliceViewer
  EXCLUDE_FROM_ALL
)
