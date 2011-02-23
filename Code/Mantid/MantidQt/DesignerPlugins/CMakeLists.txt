# Require additional Qt packages
find_package ( Qt4 COMPONENTS QtDesigner REQUIRED )
include ( ${QT_USE_FILE} )

set ( SRC_FILES src/FileFinderPlugin.cpp
                src/InstrumentSelectorPlugin.cpp
                src/PluginCollectionInterface.cpp
				src/WorkspaceSelectorPlugin.cpp )

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES inc/MantidQtDesignerPlugins/FileFinderPlugin.h
                inc/MantidQtDesignerPlugins/InstrumentSelectorPlugin.h
                inc/MantidQtDesignerPlugins/PluginCollectionInterface.h
				inc/MantidQtDesignerPlugins/WorkspaceSelectorPlugin.h )

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

add_library ( DesignerPlugins ${SRC_FILES} ${INC_FILES} ${MOCCED_FILES} )

# Change the destination of the target as Qt expects this in a directory called "designer"
if ( MSVC )
    # For some reason when the generator is MSVC  10 it ignores the  LIBRARY_OUTPUT_DIRECTORY 
    # property so we have to do something slightly different
    set ( LIB_EXTS ".dll" ".lib" ".exp" )
    set ( SRC_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} )
    set ( DEST_DIR ${SRC_DIR}/designer )
    add_custom_command( TARGET DesignerPlugins POST_BUILD 
                        COMMAND ${CMAKE_COMMAND} ARGS -E make_directory
                        ${DEST_DIR} )
    foreach ( LIB_EXT ${LIB_EXTS} )
      add_custom_command ( TARGET DesignerPlugins POST_BUILD
                           COMMAND ${CMAKE_COMMAND} ARGS -E echo 
                           "Moving \"${LIB_NAME}${LIB_EXT}\" to designer directory."
                           COMMAND ${CMAKE_COMMAND} ARGS -E rename
                           ${SRC_DIR}/${LIB_NAME}${LIB_EXT}
                           ${DEST_DIR}/${LIB_NAME}${LIB_EXT} )
    endforeach ( LIB_EXT )
    # Clean up
    set ( LIB_EXTS )
    set ( SRC_DIR )
    set ( DEST_DIR )
else ()
  set_target_properties ( DesignerPlugins PROPERTIES LIBRARY_OUTPUT_DIRECTORY 
                          ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/designer )
endif ( MSVC )
# Set the name of the generated library
set_target_properties ( DesignerPlugins PROPERTIES OUTPUT_NAME ${LIB_NAME} )

# Remove from the 'all' target as most people don't care about them
set_target_properties( DesignerPlugins PROPERTIES EXCLUDE_FROM_ALL TRUE )
# Set the library dependencies
target_link_libraries ( DesignerPlugins MantidWidgets ${QT_LIBRARIES} )

# Clean up
set ( LIB_NAME )