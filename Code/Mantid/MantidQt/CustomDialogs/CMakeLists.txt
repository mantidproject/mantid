set ( SRC_FILES src/CreateSampleShapeDialog.cpp
                src/LoadDialog.cpp
                src/LoadAsciiDialog.cpp
                src/LoadDAEDialog.cpp
                src/LoadRawDialog.cpp
                src/LOQScriptInputDialog.cpp
                src/MantidGLWidget.cpp
                src/PlotAsymmetryByLogValueDialog.cpp
                src/SampleShapeHelpers.cpp
)

set ( SRC_UNITY_IGNORE_FILES )

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES inc/MantidQtCustomDialogs/CreateSampleShapeDialog.h
                inc/MantidQtCustomDialogs/LoadDialog.h
                inc/MantidQtCustomDialogs/LoadAsciiDialog.h
                inc/MantidQtCustomDialogs/LoadDAEDialog.h
                inc/MantidQtCustomDialogs/LoadRawDialog.h
                inc/MantidQtCustomDialogs/LOQScriptInputDialog.h
                inc/MantidQtCustomDialogs/MantidGLWidget.h
                inc/MantidQtCustomDialogs/PlotAsymmetryByLogValueDialog.h
                inc/MantidQtCustomDialogs/SampleShapeHelpers.h
)

set ( MOC_FILES inc/MantidQtCustomDialogs/CreateSampleShapeDialog.h
                inc/MantidQtCustomDialogs/LoadDialog.h
                inc/MantidQtCustomDialogs/LoadAsciiDialog.h
                inc/MantidQtCustomDialogs/LoadDAEDialog.h
                inc/MantidQtCustomDialogs/LoadRawDialog.h
                inc/MantidQtCustomDialogs/LOQScriptInputDialog.h
                inc/MantidQtCustomDialogs/MantidGLWidget.h
                inc/MantidQtCustomDialogs/PlotAsymmetryByLogValueDialog.h
                inc/MantidQtCustomDialogs/SampleShapeHelpers.h
)

set ( UI_FILES inc/MantidQtCustomDialogs/CreateSampleShapeDialog.ui
               inc/MantidQtCustomDialogs/DiagScriptInputDialog.ui
               inc/MantidQtCustomDialogs/LOQScriptInputDialog.ui
               inc/MantidQtCustomDialogs/PlotAsymmetryByLogValueDialog.ui
)

include_directories ( inc )
include_directories ( ../MantidWidgets/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../MantidWidgets )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../API )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(CustomDialogs SRC_FILES SRC_UNITY_IGNORE_FILES 1000)
endif(UNITY_BUILD)


add_library ( CustomDialogs ${SRC_FILES} ${INC_FILES} ${MOCCED_FILES} ${UI_HDRS} )
# Set the name of the generated library
set_target_properties ( CustomDialogs PROPERTIES OUTPUT_NAME MantidQtCustomDialogs )

target_link_libraries ( CustomDialogs MantidQtAPI MantidWidgets )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CustomDialogs DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
