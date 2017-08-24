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

# Generate moc wrappers
qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

include_directories ( inc )
include_directories ( ../../sliceviewer/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../../sliceviewer )

include_directories ( ${QSCINTILLA_INCLUDE_DIR} )
add_definitions ( -DQSCINTILLA_DLL )     # Will only have an effect on Windows (as is desired)

# Define a compiler variable to set the name of the library within the code. This
# is required by the Qt plugin mechanism
set( LIB_NAME MantidWidgetPlugins )
add_definitions( -DLIBRARY_NAME=${LIB_NAME} )

# Create the library
add_library ( MantidQtWidgetsPluginsDesigner ${SRC_FILES} ${INC_FILES} ${MOCCED_FILES} )

# Change the destination of the target as Qt expects this in a directory called "designer"
# if you set QT_PLUGIN_PATH environment variable this might put it there???
SET_TARGET_OUTPUT_DIRECTORY( MantidQtWidgetsPluginsDesigner ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/designer )

# Set the name of the generated library
set_target_properties ( MantidQtWidgetsPluginsDesigner PROPERTIES OUTPUT_NAME ${LIB_NAME} )

# Remove from the 'all' target as most people don't care about them
set_target_properties( MantidQtWidgetsPluginsDesigner PROPERTIES EXCLUDE_FROM_ALL TRUE )

# Set the library dependencies
target_link_libraries ( MantidQtWidgetsPluginsDesigner LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} 
            MantidQtWidgetsCommonQt4
            MantidQtWidgetsSliceViewer
            ${QT_LIBRARIES}
            ${QWT_LIBRARIES}
            ${QSCINTILLA_LIBRARIES}
            ${CORE_MANTIDLIBS}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${QT_LIBRARIES} )

# Clean up
set ( LIB_NAME )
