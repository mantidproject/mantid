# Require additional Qt packages
find_package ( Qt4 COMPONENTS QtDesigner REQUIRED )
include ( ${QT_USE_FILE} )

set ( SRC_FILES 
    src/FileFinderPlugin.cpp
    src/FitBrowserPlugin.cpp 
    src/InstrumentSelectorPlugin.cpp
    src/ScriptEditorPlugin.cpp
    src/MuonFitBrowserPlugin.cpp
    src/PluginCollectionInterface.cpp
    src/WorkspaceSelectorPlugin.cpp 
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES 
    inc/MantidQtDesignerPlugins/FileFinderPlugin.h
    inc/MantidQtDesignerPlugins/FitBrowserPlugin.h
    inc/MantidQtDesignerPlugins/InstrumentSelectorPlugin.h
    inc/MantidQtDesignerPlugins/ScriptEditorPlugin.h
	inc/MantidQtDesignerPlugins/MuonFitBrowserPlugin.h
    inc/MantidQtDesignerPlugins/PluginCollectionInterface.h
	inc/MantidQtDesignerPlugins/WorkspaceSelectorPlugin.h 
)

set ( MOC_FILES ${INC_FILES} )

# Generate moc wrappers
qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

include_directories ( inc )
include_directories ( ../MantidWidgets/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../MantidWidgets )

# Define a compiler variable to set the name of the library within the code. This
# is required by the Qt plugin mechanism
set( LIB_NAME MantidWidgetPlugins )
add_definitions( -DLIBRARY_NAME=${LIB_NAME} )

set ( SRC_UNITY_IGNORE_FILES )
if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(DesignerPlugins SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

add_library ( DesignerPlugins ${SRC_FILES} ${INC_FILES} ${MOCCED_FILES} )

# Change the destination of the target as Qt expects this in a directory called "designer"
# This finds the qt plugins install folder. "/usr/lib/qt4/plugins" for me.
_qt4_query_qmake(QT_INSTALL_PLUGINS qt_plugins_dir)
SET_TARGET_OUTPUT_DIRECTORY( DesignerPlugins ${qt_plugins_dir}/designer )

# Set the name of the generated library
set_target_properties ( DesignerPlugins PROPERTIES OUTPUT_NAME ${LIB_NAME} )

# Remove from the 'all' target as most people don't care about them
set_target_properties( DesignerPlugins PROPERTIES EXCLUDE_FROM_ALL TRUE )
# Set the library dependencies
target_link_libraries ( DesignerPlugins MantidWidgets ${QT_LIBRARIES} )

# Clean up
set ( LIB_NAME )
