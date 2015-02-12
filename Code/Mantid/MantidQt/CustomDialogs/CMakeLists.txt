set ( SRC_FILES src/CatalogPublishDialog.cpp
                src/CreateSampleShapeDialog.cpp
                src/ConvertTableToMatrixWorkspaceDialog.cpp
                src/StartLiveDataDialog.cpp
                src/FitDialog.cpp
                src/LoadDialog.cpp
                src/LoadAsciiDialog.cpp
                src/LoadDAEDialog.cpp
                src/LoadInstrumentDialog.cpp               
                src/LoadRawDialog.cpp
                src/LOQScriptInputDialog.cpp
                src/MantidGLWidget.cpp
                src/PlotAsymmetryByLogValueDialog.cpp
                src/SampleShapeHelpers.cpp
                src/SmoothNeighboursDialog.cpp
                src/SortTableWorkspaceDialog.cpp
)

set ( SRC_UNITY_IGNORE_FILES )


set ( MOC_FILES inc/MantidQtCustomDialogs/CatalogPublishDialog.h
                inc/MantidQtCustomDialogs/CreateSampleShapeDialog.h
                inc/MantidQtCustomDialogs/ConvertTableToMatrixWorkspaceDialog.h
                inc/MantidQtCustomDialogs/StartLiveDataDialog.h
                inc/MantidQtCustomDialogs/FitDialog.h
                inc/MantidQtCustomDialogs/LoadDialog.h
                inc/MantidQtCustomDialogs/LoadAsciiDialog.h
                inc/MantidQtCustomDialogs/LoadDAEDialog.h
                inc/MantidQtCustomDialogs/LoadInstrumentDialog.h               
                inc/MantidQtCustomDialogs/LoadRawDialog.h
                inc/MantidQtCustomDialogs/LOQScriptInputDialog.h
                inc/MantidQtCustomDialogs/MantidGLWidget.h
                inc/MantidQtCustomDialogs/PlotAsymmetryByLogValueDialog.h
                inc/MantidQtCustomDialogs/SampleShapeHelpers.h
                inc/MantidQtCustomDialogs/SmoothNeighboursDialog.h
                inc/MantidQtCustomDialogs/SortTableWorkspaceDialog.h
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES 
    ${MOC_FILES}
)

set ( UI_FILES inc/MantidQtCustomDialogs/CatalogPublishDialog.ui
               inc/MantidQtCustomDialogs/CreateSampleShapeDialog.ui
               inc/MantidQtCustomDialogs/ConvertTableToMatrixWorkspaceDialog.ui
               inc/MantidQtCustomDialogs/DiagScriptInputDialog.ui
               inc/MantidQtCustomDialogs/LOQScriptInputDialog.ui
               inc/MantidQtCustomDialogs/PlotAsymmetryByLogValueDialog.ui
               inc/MantidQtCustomDialogs/LoadDialog.ui
               inc/MantidQtCustomDialogs/LoadInstrumentDialog.ui              
               inc/MantidQtCustomDialogs/StartLiveDataDialog.ui
               inc/MantidQtCustomDialogs/FitDialog.ui
               inc/MantidQtCustomDialogs/SortTableWorkspaceDialog.ui
)

include_directories ( inc )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(CustomDialogs SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidQtCustomDialogs/PrecompiledHeader.h ALL_SRC )
add_library ( CustomDialogs ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )
# Set the name of the generated library
set_target_properties ( CustomDialogs PROPERTIES OUTPUT_NAME MantidQtCustomDialogs )

target_link_libraries ( CustomDialogs MantidQtAPI MantidWidgets )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CustomDialogs ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
