set ( SRC_FILES 
                src/ApproachDialog.cpp
                src/background.cpp
                src/ConvertToEnergy.cpp
                src/CreateMDWorkspace.cpp
                src/deltaECalc.cpp
                src/Homer.cpp
                src/Indirect.cpp
                src/IndirectDataAnalysis.cpp
                src/IndirectDiffractionReduction.cpp
                src/IO_MuonGrouping.cpp
                src/InelasticISIS.cpp
                src/QtWorkspaceMementoModel.cpp
                src/LatticePresenter.cpp
                src/LoanedMemento.cpp
                src/LogPresenter.cpp
                src/MuonAnalysis.cpp
                src/MuonAnalysisHelper.cpp
                src/MuonAnalysisOptionTab.cpp
                src/ParameterisedLatticeView.cpp
                src/SANSAddFiles.cpp
                src/SANSPlotSpecial.cpp
                src/SANSRunWindow.cpp
                src/SANSDiagnostics.cpp
                src/SingleOwnerLock.cpp
                src/WorkspaceMementoCollection.cpp
                src/WorkspaceMementoService.cpp
                src/WorkspaceMemento.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES 
                inc/MantidQtCustomInterfaces/AbstractMementoItem.h
                inc/MantidQtCustomInterfaces/Approach.h
                inc/MantidQtCustomInterfaces/ApproachDialog.h
                inc/MantidQtCustomInterfaces/Background.h
                inc/MantidQtCustomInterfaces/ConvertToEnergy.h
                inc/MantidQtCustomInterfaces/CreateMDWorkspace.h
                inc/MantidQtCustomInterfaces/deltaECalc.h
                inc/MantidQtCustomInterfaces/Updateable.h
                inc/MantidQtCustomInterfaces/Homer.h
                inc/MantidQtCustomInterfaces/Indirect.h
                inc/MantidQtCustomInterfaces/IndirectDataAnalysis.h
                inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.h
                inc/MantidQtCustomInterfaces/IO_MuonGrouping.h
                inc/MantidQtCustomInterfaces/InelasticISIS.h
                inc/MantidQtCustomInterfaces/LatticePresenter.h
                inc/MantidQtCustomInterfaces/LatticeView.h
                inc/MantidQtCustomInterfaces/LoanedMemento.h
                inc/MantidQtCustomInterfaces/LogPresenter.h
                inc/MantidQtCustomInterfaces/LogView.h
                inc/MantidQtCustomInterfaces/MuonAnalysis.h
                inc/MantidQtCustomInterfaces/MuonAnalysisHelper.h
                inc/MantidQtCustomInterfaces/MuonAnalysisOptionTab.h
                inc/MantidQtCustomInterfaces/ParameterisedLatticeView.h
                inc/MantidQtCustomInterfaces/QtWorkspaceMementoModel.h
                inc/MantidQtCustomInterfaces/SANSAddFiles.h
                inc/MantidQtCustomInterfaces/SANSPlotSpecial.h
                inc/MantidQtCustomInterfaces/SANSRunWindow.h
                inc/MantidQtCustomInterfaces/SANSDiagnostics.h
                inc/MantidQtCustomInterfaces/SingleOwnerLock.h
                inc/MantidQtCustomInterfaces/WorkspaceMementoCollection.h
                inc/MantidQtCustomInterfaces/WorkspaceMementoLock.h
                inc/MantidQtCustomInterfaces/WorkspaceMemento.h
                inc/MantidQtCustomInterfaces/WorkspaceMementoService.h
                inc/MantidQtCustomInterfaces/WorkspaceMementoItem.h
)

set ( SRC_UNITY_IGNORE_FILES )

set ( MOC_FILES inc/MantidQtCustomInterfaces/ApproachDialog.h
                inc/MantidQtCustomInterfaces/Background.h
                inc/MantidQtCustomInterfaces/ConvertToEnergy.h
                inc/MantidQtCustomInterfaces/CreateMDWorkspace.h
                inc/MantidQtCustomInterfaces/Homer.h
                inc/MantidQtCustomInterfaces/Indirect.h
                inc/MantidQtCustomInterfaces/IndirectDataAnalysis.h
                inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.h
                inc/MantidQtCustomInterfaces/MuonAnalysis.h
                inc/MantidQtCustomInterfaces/MuonAnalysisOptionTab.h
                inc/MantidQtCustomInterfaces/ParameterisedLatticeView.h
                inc/MantidQtCustomInterfaces/SANSAddFiles.h
                inc/MantidQtCustomInterfaces/SANSPlotSpecial.h
                inc/MantidQtCustomInterfaces/SANSRunWindow.h
                inc/MantidQtCustomInterfaces/SANSDiagnostics.h
)

set ( UI_FILES inc/MantidQtCustomInterfaces/ConvertToEnergy.ui
               inc/MantidQtCustomInterfaces/CreateMDWorkspace.ui
               inc/MantidQtCustomInterfaces/IndirectDataAnalysis.ui
               inc/MantidQtCustomInterfaces/IndirectDiffractionReduction.ui
               inc/MantidQtCustomInterfaces/MuonAnalysis.ui
               inc/MantidQtCustomInterfaces/SANSPlotSpecial.ui
               inc/MantidQtCustomInterfaces/SANSRunWindow.ui
)

set( TEST_FILES
               test/AbstractMementoItemTest.h
               test/LatticePresenterTest.h
               test/LoanedMementoTest.h
               #test/LogPresenterTest.h
               test/InelasticISISTest.h
               test/SingleOwnerLockTest.h
               test/WorkspaceMementoCollectionTest.h
               test/WorkspaceMementoItemTest.h
               test/WorkspaceMementoTest.h
               test/WorkspaceMementoServiceTest.h
)

include_directories ( inc )
include_directories ( ../MantidWidgets/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../MantidWidgets )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../API )
include_directories ( ${QWT_INCLUDE_DIR} )
include_directories ( ../../QtPropertyBrowser/src )

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

target_link_libraries ( CustomInterfaces MantidQtAPI MantidWidgets QtPropertyBrowser )

if ( CXXTEST_FOUND )
  
  include_directories( inc ../../Framework/TestHelpers/inc ../../Framework/DataObjects/inc )
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( CustomInterfacesTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( CustomInterfacesTest CustomInterfaces ${MANTIDLIBS}
         ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES}
         TestHelpers DataObjects)

    add_dependencies( FrameworkTests CustomInterfacesTest )
    # Add to the 'UnitTests' group in VS
    set_property( TARGET CustomInterfacesTest PROPERTY FOLDER "UnitTests" )
  endif ()

endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS CustomInterfaces DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
