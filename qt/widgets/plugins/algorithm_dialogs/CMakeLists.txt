set ( SRC_FILES src/CatalogPublishDialog.cpp
                src/CreateSampleShapeDialog.cpp
                src/ConvertTableToMatrixWorkspaceDialog.cpp
                src/StartLiveDataDialog.cpp
                src/FitDialog.cpp
                src/LoadDialog.cpp
                src/LoadAsciiDialog.cpp
                src/LoadDAEDialog.cpp
                src/LoadRawDialog.cpp
                src/LOQScriptInputDialog.cpp
                src/MantidGLWidget.cpp
                src/GetNegMuMuonicXRDDialog.cpp
                src/PlotAsymmetryByLogValueDialog.cpp
                src/SampleShapeHelpers.cpp
                src/SmoothNeighboursDialog.cpp
                src/SortTableWorkspaceDialog.cpp
)

set ( SRC_UNITY_IGNORE_FILES )


set ( MOC_FILES inc/MantidQtWidgets/Plugins/AlgorithmDialogs/CatalogPublishDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/CreateSampleShapeDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/ConvertTableToMatrixWorkspaceDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/StartLiveDataDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/FitDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LoadDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LoadAsciiDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LoadDAEDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LoadRawDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LOQScriptInputDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/MantidGLWidget.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/GetNegMuMuonicXRDDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/PlotAsymmetryByLogValueDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/SampleShapeHelpers.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/SmoothNeighboursDialog.h
                inc/MantidQtWidgets/Plugins/AlgorithmDialogs/SortTableWorkspaceDialog.h
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES 
    ${MOC_FILES}
)

set ( UI_FILES inc/MantidQtWidgets/Plugins/AlgorithmDialogs/CatalogPublishDialog.ui
               inc/MantidQtWidgets/Plugins/AlgorithmDialogs/CreateSampleShapeDialog.ui
               inc/MantidQtWidgets/Plugins/AlgorithmDialogs/ConvertTableToMatrixWorkspaceDialog.ui
               inc/MantidQtWidgets/Plugins/AlgorithmDialogs/DiagScriptInputDialog.ui
               inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LOQScriptInputDialog.ui
               inc/MantidQtWidgets/Plugins/AlgorithmDialogs/PlotAsymmetryByLogValueDialog.ui
               inc/MantidQtWidgets/Plugins/AlgorithmDialogs/LoadDialog.ui
               inc/MantidQtWidgets/Plugins/AlgorithmDialogs/StartLiveDataDialog.ui
               inc/MantidQtWidgets/Plugins/AlgorithmDialogs/FitDialog.ui
               inc/MantidQtWidgets/Plugins/AlgorithmDialogs/SortTableWorkspaceDialog.ui
)

include_directories ( inc inc/MantidQtWidgets/Plugins/AlgorithmDialogs )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} OPTIONS -p.)

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MantidQtWidgetsPluginsAlgorithmDialogs SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidQtWidgets/Plugins/AlgorithmDialogs/PrecompiledHeader.h ALL_SRC )
add_library ( MantidQtWidgetsPluginsAlgorithmDialogs ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtWidgetsPluginsAlgorithmDialogs PROPERTIES INSTALL_RPATH "@loader_path/../../../Contents/MacOS")
endif () 

target_link_libraries ( MantidQtWidgetsPluginsAlgorithmDialogs LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} 
            MantidQtWidgetsCommon
            ${CORE_MANTIDLIBS}
            ${QT_LIBRARIES}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${OPENGL_LIBRARIES}
            )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtWidgetsPluginsAlgorithmDialogs ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
