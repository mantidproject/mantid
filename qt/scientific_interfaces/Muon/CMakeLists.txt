set ( SRC_FILES
	ALCBaselineModellingModel.cpp
	ALCBaselineModellingPresenter.cpp
	ALCBaselineModellingView.cpp
	ALCDataLoadingPresenter.cpp
	ALCDataLoadingView.cpp
	ALCInterface.cpp
	ALCLatestFileFinder.cpp
	ALCPeakFittingModel.cpp
	ALCPeakFittingPresenter.cpp
	ALCPeakFittingView.cpp
	IO_MuonGrouping.cpp
	MuonAnalysis.cpp
	MuonAnalysisDataLoader.cpp
	MuonAnalysisFitDataPresenter.cpp
	MuonAnalysisFitDataTab.cpp
	MuonAnalysisFitFunctionPresenter.cpp
	MuonAnalysisHelper.cpp
	MuonAnalysisOptionTab.cpp
	MuonAnalysisResultTableCreator.cpp
	MuonAnalysisResultTableTab.cpp
	MuonSequentialFitDialog.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
	ALCBaselineModellingModel.h
	ALCBaselineModellingPresenter.h
	ALCBaselineModellingView.h
	ALCDataLoadingPresenter.h
	ALCDataLoadingView.h
	ALCInterface.h
	ALCLatestFileFinder.h
	ALCPeakFittingModel.h
	ALCPeakFittingPresenter.h
	ALCPeakFittingView.h
        DllConfig.h
	IALCBaselineModellingModel.h
	IALCBaselineModellingView.h
	IALCDataLoadingView.h
	IALCPeakFittingModel.h
	IALCPeakFittingView.h
	IO_MuonGrouping.h
	MuonAnalysis.h
	MuonAnalysisDataLoader.h
	MuonAnalysisFitDataPresenter.h
	MuonAnalysisFitDataTab.h
	MuonAnalysisFitFunctionPresenter.h
	MuonAnalysisHelper.h
	MuonAnalysisOptionTab.h
	MuonAnalysisResultTableCreator.h
	MuonAnalysisResultTableTab.h
	MuonSequentialFitDialog.h
)

set ( MOC_FILES
    ALCBaselineModellingPresenter.h
    ALCBaselineModellingView.h
    ALCDataLoadingPresenter.h
    ALCInterface.h
    ALCPeakFittingPresenter.h
    IALCBaselineModellingView.h
    IALCBaselineModellingModel.h
    IALCDataLoadingView.h
    IALCPeakFittingView.h
    IALCPeakFittingModel.h
    MuonAnalysis.h
    MuonAnalysisFitDataPresenter.h
    MuonAnalysisFitDataTab.h
    MuonAnalysisFitFunctionPresenter.h
    MuonAnalysisHelper.h
    MuonAnalysisOptionTab.h
    MuonAnalysisResultTableTab.h
    MuonSequentialFitDialog.h
)

set ( UI_FILES
    ALCBaselineModellingView.ui
    ALCDataLoadingView.ui
    ALCInterface.ui
    ALCPeakFittingView.ui
    MuonAnalysis.ui
    MuonSequentialFitDialog.ui
)

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} OPTIONS ${extra_options} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )
include_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../MultiDatasetFit
  ${CMAKE_CURRENT_LIST_DIR}/../MultiDatasetFit
)


# Use a precompiled header where they are supported
enable_precompiled_headers( PrecompiledHeader.h  ALL_SRC )
add_library ( MantidScientificInterfacesMuon ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# Set the name of the generated library
set_target_properties ( MantidScientificInterfacesMuon PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_MUONINTERFACE )
if ( MSVC_IDE )
#  # Add to the 'ScientificInterfaces' group in VS
  set_property ( TARGET MantidScientificInterfacesMuon PROPERTY FOLDER "ScientificInterfaces" )
endif()

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidScientificInterfacesMuon PROPERTIES INSTALL_RPATH "@loader_path/../../../Contents/MacOS")
endif ()

find_Package(Threads)

target_link_libraries ( MantidScientificInterfacesMuon LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsCommonQt4
            MantidScientificInterfacesMultiDatasetFitQt4
            ${CORE_MANTIDLIBS}
            ${QT_LIBRARIES}
            ${QWT_LIBRARIES}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${JSONCPP_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT})

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidScientificInterfacesMuon ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
