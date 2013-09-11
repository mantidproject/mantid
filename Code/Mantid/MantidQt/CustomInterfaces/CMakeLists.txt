set ( SRC_FILES
    src/ApplyCorr.cpp
    src/C2ETab.cpp
    src/CalcCorr.cpp
    src/ConvertToEnergy.cpp
    src/ConvFit.cpp
	src/CreateMDWorkspace.cpp
	src/CreateMDWorkspaceAlgDialog.cpp	
    src/Elwin.cpp
	src/EventNexusFileMemento.cpp
    src/Fury.cpp
    src/FuryFit.cpp
	src/Homer.cpp
    src/IDATab.cpp
	src/IO_MuonGrouping.cpp
	src/Indirect.cpp
	src/IndirectBayes.cpp
	src/IndirectBayesTab.cpp
	src/IndirectDataAnalysis.cpp
	src/IndirectDiffractionReduction.cpp
	src/JumpFit.cpp
    src/MSDFit.cpp
	src/MuonAnalysis.cpp
	src/MuonAnalysisFitDataTab.cpp
	src/MuonAnalysisHelper.cpp
	src/MuonAnalysisOptionTab.cpp
	src/MuonAnalysisResultTableTab.cpp
	src/QtWorkspaceMementoModel.cpp
	src/Quasi.cpp
	src/ResNorm.cpp
	src/SANSAddFiles.cpp
	src/SANSDiagnostics.cpp
	src/SANSPlotSpecial.cpp
	src/SANSRunWindow.cpp
	src/Stretch.cpp
	src/Transmission.cpp
	src/MantidEV.cpp
	src/MantidEVWorker.cpp
	src/StepScan.cpp
	src/background.cpp
	src/deltaECalc.cpp
	src/UserInputValidator.cpp
	src/WorkspaceInADS.cpp
	src/WorkspaceMemento.cpp
	src/RawFileMemento.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	inc/MantidQtCustomInterfaces/Background.h
    inc/MantidQtCustomInterfaces/ApplyCorr.h
    inc/MantidQtCustomInterfaces/C2ETab.h
    inc/MantidQtCustomInterfaces/CalcCorr.h
	inc/MantidQtCustomInterfaces/ConvertToEnergy.h
    inc/MantidQtCustomInterfaces/ConvFit.h
	inc/MantidQtCustomInterfaces/CreateMDWorkspace.h
    inc/MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.h
    inc/MantidQtCustomInterfaces/DllConfig.h
    inc/MantidQtCustomInterfaces/Elwin.h
	inc/MantidQtCustomInterfaces/EventNexusFileMemento.h	
    inc/MantidQtCustomInterfaces/Fury.h
    inc/MantidQtCustomInterfaces/FuryFit.h
	inc/MantidQtCustomInterfaces/Homer.h
    inc/MantidQtCustomInterfaces/IDATab.h
	inc/MantidQtCustomInterfaces/IO_MuonGrouping.h
	inc/MantidQtCustomInterfaces/Indirect.h
	inc/MantidQtCustomInterfaces/IndirectBayes.h
	inc/MantidQtCustomInterfaces/IndirectBayesTab.h
	inc/MantidQtCustomInterfaces/IndirectDataAnalysis.h
	inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.h
	inc/MantidQtCustomInterfaces/JumpFit.h
    inc/MantidQtCustomInterfaces/MSDFit.h
	inc/MantidQtCustomInterfaces/MuonAnalysis.h
	inc/MantidQtCustomInterfaces/MuonAnalysisFitDataTab.h
	inc/MantidQtCustomInterfaces/MuonAnalysisHelper.h
	inc/MantidQtCustomInterfaces/MuonAnalysisOptionTab.h
	inc/MantidQtCustomInterfaces/MuonAnalysisResultTableTab.h
	inc/MantidQtCustomInterfaces/QtWorkspaceMementoModel.h
	inc/MantidQtCustomInterfaces/Quasi.h
	inc/MantidQtCustomInterfaces/ResNorm.h
	inc/MantidQtCustomInterfaces/SANSAddFiles.h
	inc/MantidQtCustomInterfaces/SANSDiagnostics.h
	inc/MantidQtCustomInterfaces/SANSPlotSpecial.h
	inc/MantidQtCustomInterfaces/SANSRunWindow.h
	inc/MantidQtCustomInterfaces/Stretch.h
	inc/MantidQtCustomInterfaces/Transmission.h
	inc/MantidQtCustomInterfaces/MantidEV.h
	inc/MantidQtCustomInterfaces/MantidEVWorker.h
	inc/MantidQtCustomInterfaces/StepScan.h
	inc/MantidQtCustomInterfaces/Updateable.h
	inc/MantidQtCustomInterfaces/WorkspaceMemento.h
	inc/MantidQtCustomInterfaces/RawFileMemento.h
	inc/MantidQtCustomInterfaces/WorkspaceInADS.h
	inc/MantidQtCustomInterfaces/UserInputValidator.h
	inc/MantidQtCustomInterfaces/deltaECalc.h
)

set ( SRC_UNITY_IGNORE_FILES )

set ( MOC_FILES inc/MantidQtCustomInterfaces/Background.h
                inc/MantidQtCustomInterfaces/ApplyCorr.h
                inc/MantidQtCustomInterfaces/C2ETab.h
                inc/MantidQtCustomInterfaces/CalcCorr.h
                inc/MantidQtCustomInterfaces/ConvertToEnergy.h
                inc/MantidQtCustomInterfaces/ConvFit.h
                inc/MantidQtCustomInterfaces/CreateMDWorkspace.h
                inc/MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.h
                inc/MantidQtCustomInterfaces/Elwin.h
                inc/MantidQtCustomInterfaces/Fury.h
                inc/MantidQtCustomInterfaces/FuryFit.h
                inc/MantidQtCustomInterfaces/Homer.h
                inc/MantidQtCustomInterfaces/IDATab.h
                inc/MantidQtCustomInterfaces/Indirect.h
								inc/MantidQtCustomInterfaces/IndirectBayes.h
								inc/MantidQtCustomInterfaces/IndirectBayesTab.h
                inc/MantidQtCustomInterfaces/IndirectDataAnalysis.h
                inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.h
                inc/MantidQtCustomInterfaces/JumpFit.h
                inc/MantidQtCustomInterfaces/MSDFit.h
                inc/MantidQtCustomInterfaces/MuonAnalysis.h
                inc/MantidQtCustomInterfaces/MuonAnalysisFitDataTab.h
                inc/MantidQtCustomInterfaces/MuonAnalysisOptionTab.h
                inc/MantidQtCustomInterfaces/MuonAnalysisResultTableTab.h
                inc/MantidQtCustomInterfaces/Quasi.h
								inc/MantidQtCustomInterfaces/ResNorm.h
                inc/MantidQtCustomInterfaces/SANSAddFiles.h
                inc/MantidQtCustomInterfaces/SANSPlotSpecial.h
                inc/MantidQtCustomInterfaces/SANSRunWindow.h
                inc/MantidQtCustomInterfaces/SANSDiagnostics.h
                inc/MantidQtCustomInterfaces/Stretch.h
                inc/MantidQtCustomInterfaces/Transmission.h
								inc/MantidQtCustomInterfaces/MantidEV.h
                inc/MantidQtCustomInterfaces/StepScan.h
)

set ( UI_FILES inc/MantidQtCustomInterfaces/ConvertToEnergy.ui
               inc/MantidQtCustomInterfaces/CreateMDWorkspace.ui
               inc/MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.ui
               inc/MantidQtCustomInterfaces/IndirectBayes.ui
               inc/MantidQtCustomInterfaces/IndirectDataAnalysis.ui
               inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.ui
               inc/MantidQtCustomInterfaces/JumpFit.ui
               inc/MantidQtCustomInterfaces/MuonAnalysis.ui
               inc/MantidQtCustomInterfaces/Quasi.ui
               inc/MantidQtCustomInterfaces/ResNorm.ui
               inc/MantidQtCustomInterfaces/SANSPlotSpecial.ui
               inc/MantidQtCustomInterfaces/SANSRunWindow.ui	
               inc/MantidQtCustomInterfaces/Stretch.ui
               inc/MantidQtCustomInterfaces/MantidEV.ui
               inc/MantidQtCustomInterfaces/StepScan.ui
)

set( TEST_FILES CreateMDWorkspaceAlgDialogTest.h
                EventNexusFileMementoTest.h
                WorkspaceMementoTest.h
                WorkspaceInADSTest.h
                RawFileMementoTest.h
)

include_directories ( inc )
include_directories ( ../../QtPropertyBrowser/src )

# Ugly hack to get around issue in Boost version 1.48.0 
# in conjunction with Qt 4.7.4 or greater
set ( qt_version ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH} )
if ( ${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3 )
	set( extra_options "-DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED")
else ( ${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3 )
	set ( extra_options "" )
endif ( ${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3 )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} OPTIONS ${extra_options} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(CustomInterfaces SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidQtCustomInterfaces/PrecompiledHeader.h  ALL_SRC )
add_library ( CustomInterfaces ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# Set the name of the generated library
set_target_properties ( CustomInterfaces PROPERTIES OUTPUT_NAME MantidQtCustomInterfaces
                                                    COMPILE_DEFINITIONS IN_MANTIDQT_CUSTOMINTERFACES )

find_Package(Threads)

target_link_libraries ( CustomInterfaces MantidQtAPI MantidWidgets QtPropertyBrowser ${CMAKE_THREAD_LIBS_INIT})

###########################################################################
# Testing
###########################################################################

add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CustomInterfaces ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
