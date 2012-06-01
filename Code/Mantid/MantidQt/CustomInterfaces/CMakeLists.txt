set ( SRC_FILES
    src/ApplyCorr.cpp
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
	src/IndirectDataAnalysis.cpp
	src/IndirectDiffractionReduction.cpp
    src/MSDFit.cpp
	src/MuonAnalysis.cpp
	src/MuonAnalysisFitDataTab.cpp
	src/MuonAnalysisHelper.cpp
	src/MuonAnalysisOptionTab.cpp
	src/MuonAnalysisResultTableTab.cpp
	src/QtWorkspaceMementoModel.cpp
	src/SANSAddFiles.cpp
	src/SANSDiagnostics.cpp
	src/SANSPlotSpecial.cpp
	src/SANSRunWindow.cpp
	src/background.cpp
	src/deltaECalc.cpp
	src/WorkspaceInADS.cpp
	src/WorkspaceMemento.cpp
	src/RawFileMemento.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	inc/MantidQtCustomInterfaces/Background.h
    inc/MantidQtCustomInterfaces/ApplyCorr.h
    inc/MantidQtCustomInterfaces/CalcCorr.h
	inc/MantidQtCustomInterfaces/ConvertToEnergy.h
    inc/MantidQtCustomInterfaces/ConvFit.h
	inc/MantidQtCustomInterfaces/CreateMDWorkspace.h
    inc/MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.h
    inc/MantidQtCustomInterfaces/Elwin.h
	inc/MantidQtCustomInterfaces/EventNexusFileMemento.h	
    inc/MantidQtCustomInterfaces/Fury.h
    inc/MantidQtCustomInterfaces/FuryFit.h
	inc/MantidQtCustomInterfaces/Homer.h
    inc/MantidQtCustomInterfaces/IDATab.h
	inc/MantidQtCustomInterfaces/IO_MuonGrouping.h
	inc/MantidQtCustomInterfaces/Indirect.h
	inc/MantidQtCustomInterfaces/IndirectDataAnalysis.h
	inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.h
    inc/MantidQtCustomInterfaces/MSDFit.h
	inc/MantidQtCustomInterfaces/MuonAnalysis.h
	inc/MantidQtCustomInterfaces/MuonAnalysisFitDataTab.h
	inc/MantidQtCustomInterfaces/MuonAnalysisHelper.h
	inc/MantidQtCustomInterfaces/MuonAnalysisOptionTab.h
	inc/MantidQtCustomInterfaces/MuonAnalysisResultTableTab.h
	inc/MantidQtCustomInterfaces/QtWorkspaceMementoModel.h
	inc/MantidQtCustomInterfaces/SANSAddFiles.h
	inc/MantidQtCustomInterfaces/SANSDiagnostics.h
	inc/MantidQtCustomInterfaces/SANSPlotSpecial.h
	inc/MantidQtCustomInterfaces/SANSRunWindow.h
	inc/MantidQtCustomInterfaces/Updateable.h
	inc/MantidQtCustomInterfaces/WorkspaceMemento.h
	inc/MantidQtCustomInterfaces/RawFileMemento.h
	inc/MantidQtCustomInterfaces/WorkspaceInADS.h
	inc/MantidQtCustomInterfaces/deltaECalc.h
)

set ( SRC_UNITY_IGNORE_FILES )

set ( MOC_FILES inc/MantidQtCustomInterfaces/Background.h
                inc/MantidQtCustomInterfaces/ApplyCorr.h
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
                inc/MantidQtCustomInterfaces/IndirectDataAnalysis.h
                inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.h
                inc/MantidQtCustomInterfaces/MSDFit.h
                inc/MantidQtCustomInterfaces/MuonAnalysis.h
                inc/MantidQtCustomInterfaces/MuonAnalysisFitDataTab.h
                inc/MantidQtCustomInterfaces/MuonAnalysisOptionTab.h
                inc/MantidQtCustomInterfaces/MuonAnalysisResultTableTab.h
                inc/MantidQtCustomInterfaces/SANSAddFiles.h
                inc/MantidQtCustomInterfaces/SANSPlotSpecial.h
                inc/MantidQtCustomInterfaces/SANSRunWindow.h
                inc/MantidQtCustomInterfaces/SANSDiagnostics.h
)

set ( UI_FILES inc/MantidQtCustomInterfaces/ConvertToEnergy.ui
               inc/MantidQtCustomInterfaces/CreateMDWorkspace.ui
               inc/MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.ui
               inc/MantidQtCustomInterfaces/IndirectDataAnalysis.ui
               inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.ui
               inc/MantidQtCustomInterfaces/MuonAnalysis.ui
               inc/MantidQtCustomInterfaces/SANSPlotSpecial.ui
               inc/MantidQtCustomInterfaces/SANSRunWindow.ui
)

set( TEST_FILES
               test/EventNexusFileMementoTest.h
               test/WorkspaceMementoTest.h
               test/WorkspaceInADSTest.h
               test/RawFileMementoTest.h
)

include_directories ( inc )
include_directories ( ../MantidWidgets/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../MantidWidgets )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../API )
include_directories ( ../../QtPropertyBrowser/src )
# the directory with the information about ConvertToMD interface definitions
#include_directories (../../Framework/MDAlgorithms/inc)
include_directories (../../Framework/MDEvents/inc)

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(CustomInterfaces SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidQtCustomInterfaces/PrecompiledHeader.h MantidQtCustomInterfaces src/PrecompiledHeader.cpp ALL_SRC INC_FILES )
add_library ( CustomInterfaces ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# Set the name of the generated library
set_target_properties ( CustomInterfaces PROPERTIES OUTPUT_NAME MantidQtCustomInterfaces )

Find_Package(Threads)

target_link_libraries ( CustomInterfaces MantidQtAPI MantidWidgets QtPropertyBrowser MDEvents ${CMAKE_THREAD_LIBS_INIT})

if ( CXXTEST_FOUND )
  
  include_directories( ../../Framework/TestHelpers/inc ../../Framework/DataObjects/inc ../../Framework/Crystal/inc )

  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( CustomInterfacesTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( CustomInterfacesTest CustomInterfaces DataObjects ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )

    add_dependencies( GUITests CustomInterfacesTest )
    # Add to the 'UnitTests' group in VS
    set_property( TARGET CustomInterfacesTest PROPERTY FOLDER "UnitTests" )
  endif ()

endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CustomInterfaces ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
