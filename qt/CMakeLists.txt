###########################################################################
# Now add the packages one-by-one, building up the dependencies as we go
###########################################################################
find_package ( QScintilla REQUIRED )

include ( UseQt4 )
include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

# Any MantidQt package can depend on widgets/common, so add the includes to the path here
include_directories ( widgets/common/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/widgets/common )
add_subdirectory ( widgets )
add_subdirectory ( python )
add_subdirectory ( scientific_interfaces )
if ( MAKE_VATES )
  add_subdirectory ( paraview_ext )
endif ( MAKE_VATES )

###########################################################################
# Add a custom target to build all of the MantidQt packages
###########################################################################

if ( MSVC_IDE )
#  # Add to the 'MantidQt' group in VS
  set_property ( TARGET qtwidgetscommon PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET qtwidgetsinstrumentview PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET qtwidgetssliceviewer PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET qtwidgetsspectrumviewer PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET qtwidgetsrefdetectorview PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET qtwidgetsfactory PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET qtwidgetspluginsdesigner PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET qtwidgetspluginsalgorithm_dialogs PROPERTY FOLDER "MantidQt" )
  set_property ( TARGET mantidqtpython PROPERTY FOLDER "MantidQt" )
else ()
  add_custom_target ( MantidQt DEPENDS qtwidgetscommon 
                                       qtwidgetssliceviewer
                                       qtwidgetsspectrumviewer
                                       qtwidgetsrefdetectorview
                                       qtwidgetsfactory
                                       qtwidgetspluginsalgorithm_dialogs
                                       mantidqtpython
  )
endif ()
