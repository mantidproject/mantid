find_package( ParaView REQUIRED )
include( ${PARAVIEW_USE_FILE} )

if( PARAVIEW_BUILD_QT_GUI )
  # So that source files show-up in VS.
  set( INCLUDE_FILES RebinningCutterToolBarActions.h )
  # So that source files show-up in VS.
  set( SRC_FILES RebinningCutterToolBarActions.cxx ) 
  
  qt4_wrap_cpp( MOC_SRCS RebinningCutterToolBarActions.h )

  add_paraview_action_group( IFACES IFACE_SRCS CLASS_NAME RebinningCutterToolBarActions
                            GROUP_NAME "ToolBar/RebinningCutterToolBarActions")

  add_paraview_plugin( MantidParaViewRebinningCutterToolBarActions "1.0"
                       GUI_INTERFACES ${IFACES}
                       SOURCES RebinningCutterToolBarActions.cxx ${MOC_SRCS} 
                       ${IFACE_SRCS} ${INCLUDE_FILES} ${SRC_FILES} )
endif( PARAVIEW_BUILD_QT_GUI )
