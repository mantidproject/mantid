set ( SRC_FILES
	AbsorptionCorrections.cpp
	ApplyPaalmanPings.cpp
	CalculatePaalmanPings.cpp
	ContainerSubtraction.cpp
	ConvFit.cpp
	CorrectionsTab.cpp
	DensityOfStates.cpp
        Elwin.cpp
	ILLEnergyTransfer.cpp
	ISISCalibration.cpp
	ISISDiagnostics.cpp
	ISISEnergyTransfer.cpp
	IndirectBayes.cpp
	IndirectBayesTab.cpp
	IndirectCorrections.cpp
	IndirectDataAnalysis.cpp
	IndirectDataAnalysisTab.cpp
	IndirectDataReduction.cpp
	IndirectDataReductionTab.cpp
	IndirectDiffractionReduction.cpp
	IndirectLoadILL.cpp
	IndirectMolDyn.cpp
	IndirectMoments.cpp
	IndirectSassena.cpp
	IndirectSimulation.cpp
	IndirectSimulationTab.cpp
	IndirectSqw.cpp
	IndirectSymmetrise.cpp
	IndirectTab.cpp
	IndirectTools.cpp
	IndirectToolsTab.cpp
	IndirectTransmission.cpp
	IndirectTransmissionCalc.cpp
	Iqt.cpp
	IqtFit.cpp
	JumpFit.cpp
	MSDFit.cpp
	Quasi.cpp
	ResNorm.cpp
	Stretch.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
    DllConfig.h
	AbsorptionCorrections.h
	ApplyPaalmanPings.h
	CalculatePaalmanPings.h
	ContainerSubtraction.h
	ConvFit.h
	CorrectionsTab.h
	DensityOfStates.h
        Elwin.h
	ILLEnergyTransfer.h
	ISISCalibration.h
	ISISDiagnostics.h
	ISISEnergyTransfer.h
	IndirectBayes.h
	IndirectBayesTab.h
	IndirectCorrections.h
	IndirectDataAnalysis.h
	IndirectDataAnalysisTab.h
	IndirectDataReduction.h
	IndirectDataReductionTab.h
	IndirectDiffractionReduction.h
	IndirectLoadILL.h
	IndirectMolDyn.h
	IndirectMoments.h
	IndirectSassena.h
	IndirectSimulation.h
	IndirectSimulationTab.h
	IndirectSqw.h
	IndirectSymmetrise.h
	IndirectTab.h
	IndirectTools.h
	IndirectToolsTab.h
	IndirectTransmission.h
	IndirectTransmissionCalc.h
	Iqt.h
	IqtFit.h
	JumpFit.h
	MSDFit.h
	Quasi.h
	ResNorm.h
	Stretch.h
)

set ( MOC_FILES 
    AbsorptionCorrections.h
    ApplyPaalmanPings.h
    CalculatePaalmanPings.h
    ContainerSubtraction.h
    ConvFit.h
    CorrectionsTab.h
    DensityOfStates.h
    Elwin.h
    ILLEnergyTransfer.h
    IndirectBayes.h
    IndirectBayesTab.h
    IndirectCorrections.h
    IndirectDataAnalysis.h
    IndirectDataAnalysisTab.h
    IndirectDataReduction.h
    IndirectDataReductionTab.h
    IndirectDiffractionReduction.h
    IndirectLoadILL.h
    IndirectMolDyn.h
    IndirectMoments.h
    IndirectSassena.h
    IndirectSimulation.h
    IndirectSimulationTab.h
    IndirectSqw.h
    IndirectSymmetrise.h
    IndirectTransmission.h
    IndirectTransmissionCalc.h
    IndirectTools.h
    IndirectToolsTab.h
    IndirectTab.h
    Iqt.h
    IqtFit.h
    JumpFit.h
    MSDFit.h
    Quasi.h
    ResNorm.h
    Stretch.h
    ISISCalibration.h
    ISISDiagnostics.h
    ISISEnergyTransfer.h
)

set ( UI_FILES 
   AbsorptionCorrections.ui
   ApplyPaalmanPings.ui
   CalculatePaalmanPings.ui
   ContainerSubtraction.ui
   ConvFit.ui
   DensityOfStates.ui
   Elwin.ui
   ILLEnergyTransfer.ui
   IndirectBayes.ui
   IndirectCorrections.ui
   IndirectDataAnalysis.ui
   IndirectDataReduction.ui
   IndirectDiffractionReduction.ui
   IndirectLoadILL.ui
   IndirectMolDyn.ui
   IndirectMoments.ui
   IndirectSassena.ui
   IndirectSimulation.ui
   IndirectSqw.ui
   IndirectSymmetrise.ui
   IndirectTools.ui
   IndirectTransmission.ui
   IndirectTransmissionCalc.ui
   Iqt.ui
   IqtFit.ui
   JumpFit.ui
   MSDFit.ui
   Quasi.ui
   ResNorm.ui
   Stretch.ui
   ISISCalibration.ui
   ISISDiagnostics.ui
   ISISEnergyTransfer.ui
)


qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} OPTIONS ${extra_options} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# Use a precompiled header where they are supported
enable_precompiled_headers( PrecompiledHeader.h  ALL_SRC )
add_library ( MantidScientificInterfacesIndirect ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

# Set the name of the generated library
set_target_properties ( MantidScientificInterfacesIndirect PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_INDIRECT )
if ( MSVC_IDE )
#  # Add to the 'ScientificInterfaces' group in VS
  set_property ( TARGET MantidScientificInterfacesIndirect PROPERTY FOLDER "ScientificInterfaces" )
endif()

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidScientificInterfacesIndirect PROPERTIES INSTALL_RPATH "@loader_path/../../../Contents/MacOS")
endif ()

find_Package(Threads)

target_link_libraries ( MantidScientificInterfacesIndirect LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsCommonQt4
            MantidScientificInterfacesGeneralQt4
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

install ( TARGETS MantidScientificInterfacesIndirect ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR}/qtplugins/mantid )
