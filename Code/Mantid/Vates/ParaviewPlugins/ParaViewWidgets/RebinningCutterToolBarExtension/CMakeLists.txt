
FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})


IF(PARAVIEW_BUILD_QT_GUI)
  
  #So that source files show-up in VS.
  SET(INCLUDE_FILES RebinningCutterToolBarActions.h)
  #So that source files show-up in VS.
  SET(SRC_FILES RebinningCutterToolBarActions.cxx) 
  
  QT4_WRAP_CPP(MOC_SRCS RebinningCutterToolBarActions.h)

  ADD_PARAVIEW_ACTION_GROUP(IFACES IFACE_SRCS CLASS_NAME RebinningCutterToolBarActions
                            GROUP_NAME "ToolBar/RebinningCutterToolBarActions")

  ADD_PARAVIEW_PLUGIN(MantidParaViewRebinningCutterToolBarActions "1.0"
                      GUI_INTERFACES ${IFACES}
                      SOURCES RebinningCutterToolBarActions.cxx ${MOC_SRCS} ${IFACE_SRCS} ${INCLUDE_FILES} ${SRC_FILES})
ENDIF(PARAVIEW_BUILD_QT_GUI)
