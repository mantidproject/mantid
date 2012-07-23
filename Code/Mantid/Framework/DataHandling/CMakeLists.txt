set ( SRC_FILES
	#       src/LoadSassena.cpp
	src/ADARAPackets.cpp
	src/ADARAParser.cpp
	src/AppendGeometryToSNSNexus.cpp
	src/CompressEvents.cpp
	src/CreateSampleShape.cpp
	src/DefineGaugeVolume.cpp
	src/DeleteTableRows.cpp
	src/DetermineChunking.cpp
	src/FakeEventDataListener.cpp
	src/FakeISISHistoDAE.cpp
	src/FileEventDataListener.cpp
	src/FindDetectorsInShape.cpp
	src/FindDetectorsPar.cpp
	src/GenerateGroupingPowder.cpp
	src/GetMaskedDetectors.cpp
	src/GroupDetectors.cpp
	src/GroupDetectors2.cpp
	src/ISISDataArchive.cpp
	src/ISISHistoDataListener.cpp
	src/ISISRunLogs.cpp
	src/LiveDataAlgorithm.cpp
	src/Load.cpp
	src/LoadAscii.cpp
	src/LoadCalFile.cpp
	src/LoadCanSAS1D.cpp
	src/LoadDAE.cpp
	src/LoadDAE/idc.cpp
	src/LoadDAE/idc.h
	src/LoadDAE/isisds_command.cpp
	src/LoadDAE/isisds_command.h
	src/LoadDaveGrp.cpp
	src/LoadDetectorInfo.cpp
	src/LoadDetectorsGroupingFile.cpp
	src/LoadDspacemap.cpp
	src/LoadEmptyInstrument.cpp
	src/LoadEventNexus.cpp
	src/LoadEventPreNexus.cpp
	src/LoadEventPreNexus2.cpp
	src/LoadGSS.cpp
	src/LoadISISNexus.cpp
	src/LoadISISNexus2.cpp
	src/LoadILL.cpp
	src/LoadInstrument.cpp
	src/LoadInstrumentFromNexus.cpp
	src/LoadInstrumentFromRaw.cpp
	src/LoadIsawDetCal.cpp
	src/LoadLOQDistancesFromRaw.cpp
	src/LoadLiveData.cpp
	src/LoadLog.cpp
	src/LoadLogsForSNSPulsedMagnet.cpp
	src/LoadLogsFromSNSNexus.cpp
	src/LoadMappingTable.cpp
	src/LoadMask.cpp
	src/LoadMaskingFile.cpp
	src/LoadMuonLog.cpp
	src/LoadMuonNexus.cpp
	src/LoadMuonNexus2.cpp
	src/LoadNXSPE.cpp
	src/LoadNexus.cpp
	src/LoadNexusLogs.cpp
	src/LoadNexusMonitors.cpp
	src/LoadNexusProcessed.cpp
	src/LoadParameterFile.cpp
	src/LoadPreNexus.cpp
	src/LoadPreNexusMonitors.cpp
	src/LoadQKK.cpp
	src/LoadRKH.cpp
	src/LoadRaw.cpp
	src/LoadRaw/byte_rel_comp.cpp
	src/LoadRaw/isisraw.cpp
	src/LoadRaw/isisraw2.cpp
	src/LoadRaw/item_struct.cpp
	src/LoadRaw/vms_convert.cpp
	src/LoadRaw2.cpp
	src/LoadRaw3.cpp
	src/LoadRawBin0.cpp
	src/LoadRawHelper.cpp
	src/LoadRawSpectrum0.cpp
	src/LoadSNSEventNexus.cpp
	src/LoadSNSNexus.cpp
	src/LoadSNSspec.cpp
	src/LoadSPE.cpp
	src/LoadSampleDetailsFromRaw.cpp
	src/LoadSpec.cpp
	src/LoadSpice2D.cpp
	src/LoadTOFRawNexus.cpp
	src/ManagedRawFileWorkspace2D.cpp
	src/MaskDetectors.cpp
	src/MaskDetectorsInShape.cpp
	src/Merge2WorkspaceLogs.cpp
	src/MonitorLiveData.cpp
	src/MoveInstrumentComponent.cpp
	src/NexusTester.cpp
	src/OrbiterDataArchive.cpp
	src/ProcessDasNexusLog.cpp
	src/RawFileInfo.cpp
	src/RemoveLogs.cpp
	src/RenameLog.cpp
	src/RotateInstrumentComponent.cpp
	src/SNSDataArchive.cpp
	src/SNSLiveEventDataListener.cpp
	src/SaveAscii.cpp
	src/SaveCSV.cpp
	src/SaveCalFile.cpp
	src/SaveCanSAS1D.cpp
	src/SaveDASC.cpp
	src/SaveDaveGrp.cpp
	src/SaveDetectorMasks.cpp
	src/SaveDetectorsGrouping.cpp
	src/SaveDspacemap.cpp
	src/SaveFocusedXYE.cpp
	src/SaveGSS.cpp
	src/SaveISISNexus.cpp
	src/SaveIsawDetCal.cpp
	src/SaveMask.cpp
	src/SaveNISTDAT.cpp
	src/SaveNXSPE.cpp
	src/SaveNexus.cpp
	src/SaveNexusProcessed.cpp
	src/SavePAR.cpp
	src/SavePHX.cpp
	src/SaveRKH.cpp
	src/SaveSPE.cpp
	src/SaveToSNSHistogramNexus.cpp
	src/SaveVTK.cpp
	src/SetScalingPSD.cpp
	src/StartLiveData.cpp
	src/UpdateInstrumentFromFile.cpp
)

set ( SRC_UNITY_IGNORE_FILES src/LoadDAE/idc.cpp
        src/LoadDAE/isisds_command.cpp)

set ( INC_FILES
	#	inc/MantidDataHandling/LoadSassena.h
	inc/MantidDataHandling/ADARA.h
	inc/MantidDataHandling/ADARAPackets.h
	inc/MantidDataHandling/ADARAParser.h
	inc/MantidDataHandling/AppendGeometryToSNSNexus.h
	inc/MantidDataHandling/CompressEvents.h
	inc/MantidDataHandling/CreateSampleShape.h
	inc/MantidDataHandling/DefineGaugeVolume.h
	inc/MantidDataHandling/DeleteTableRows.h
	inc/MantidDataHandling/DetermineChunking.h
	inc/MantidDataHandling/FakeEventDataListener.h
	inc/MantidDataHandling/FakeISISHistoDAE.h
	inc/MantidDataHandling/FileEventDataListener.h
	inc/MantidDataHandling/FindDetectorsInShape.h
	inc/MantidDataHandling/FindDetectorsPar.h
	inc/MantidDataHandling/GenerateGroupingPowder.h
	inc/MantidDataHandling/GetMaskedDetectors.h
	inc/MantidDataHandling/GroupDetectors.h
	inc/MantidDataHandling/GroupDetectors2.h
	inc/MantidDataHandling/ISISDataArchive.h
	inc/MantidDataHandling/ISISHistoDataListener.h
	inc/MantidDataHandling/ISISRunLogs.h
	inc/MantidDataHandling/LiveDataAlgorithm.h
	inc/MantidDataHandling/Load.h
	inc/MantidDataHandling/LoadAscii.h
	inc/MantidDataHandling/LoadCalFile.h
	inc/MantidDataHandling/LoadCanSAS1D.h
	inc/MantidDataHandling/LoadDAE.h
	inc/MantidDataHandling/LoadDaveGrp.h
	inc/MantidDataHandling/LoadDetectorInfo.h
	inc/MantidDataHandling/LoadDetectorsGroupingFile.h
	inc/MantidDataHandling/LoadDspacemap.h
	inc/MantidDataHandling/LoadEmptyInstrument.h
	inc/MantidDataHandling/LoadEventNexus.h
	inc/MantidDataHandling/LoadEventPreNexus.h
	inc/MantidDataHandling/LoadEventPreNexus2.h
	inc/MantidDataHandling/LoadGSS.h
	inc/MantidDataHandling/LoadISISNexus.h
	inc/MantidDataHandling/LoadISISNexus2.h
	inc/MantidDataHandling/LoadILL.h
	inc/MantidDataHandling/LoadInstrument.h
	inc/MantidDataHandling/LoadInstrumentFromNexus.h
	inc/MantidDataHandling/LoadInstrumentFromRaw.h
	inc/MantidDataHandling/LoadIsawDetCal.h
	inc/MantidDataHandling/LoadLOQDistancesFromRaw.h
	inc/MantidDataHandling/LoadLiveData.h
	inc/MantidDataHandling/LoadLog.h
	inc/MantidDataHandling/LoadLogsForSNSPulsedMagnet.h
	inc/MantidDataHandling/LoadLogsFromSNSNexus.h
	inc/MantidDataHandling/LoadMappingTable.h
	inc/MantidDataHandling/LoadMask.h
	inc/MantidDataHandling/LoadMaskingFile.h
	inc/MantidDataHandling/LoadMuonLog.h
	inc/MantidDataHandling/LoadMuonNexus.h
	inc/MantidDataHandling/LoadMuonNexus2.h
	inc/MantidDataHandling/LoadNXSPE.h
	inc/MantidDataHandling/LoadNexus.h
	inc/MantidDataHandling/LoadNexusLogs.h
	inc/MantidDataHandling/LoadNexusMonitors.h
	inc/MantidDataHandling/LoadNexusProcessed.h
	inc/MantidDataHandling/LoadParameterFile.h
	inc/MantidDataHandling/LoadPreNexus.h
	inc/MantidDataHandling/LoadPreNexusMonitors.h
	inc/MantidDataHandling/LoadQKK.h
	inc/MantidDataHandling/LoadRKH.h
	inc/MantidDataHandling/LoadRaw.h
	inc/MantidDataHandling/LoadRaw2.h
	inc/MantidDataHandling/LoadRaw3.h
	inc/MantidDataHandling/LoadRawBin0.h
	inc/MantidDataHandling/LoadRawHelper.h
	inc/MantidDataHandling/LoadRawSpectrum0.h
	inc/MantidDataHandling/LoadSNSEventNexus.h
	inc/MantidDataHandling/LoadSNSNexus.h
	inc/MantidDataHandling/LoadSNSspec.h
	inc/MantidDataHandling/LoadSPE.h
	inc/MantidDataHandling/LoadSampleDetailsFromRaw.h
	inc/MantidDataHandling/LoadSpec.h
	inc/MantidDataHandling/LoadSpice2D.h
	inc/MantidDataHandling/LoadTOFRawNexus.h
	inc/MantidDataHandling/ManagedRawFileWorkspace2D.h
	inc/MantidDataHandling/MaskDetectors.h
	inc/MantidDataHandling/MaskDetectorsInShape.h
	inc/MantidDataHandling/Merge2WorkspaceLogs.h
	inc/MantidDataHandling/MonitorLiveData.h
	inc/MantidDataHandling/MoveInstrumentComponent.h
	inc/MantidDataHandling/NexusTester.h
	inc/MantidDataHandling/OrbiterDataArchive.h
	inc/MantidDataHandling/ProcessDasNexusLog.h
	inc/MantidDataHandling/RawFileInfo.h
	inc/MantidDataHandling/RemoveLogs.h
	inc/MantidDataHandling/RenameLog.h
	inc/MantidDataHandling/RotateInstrumentComponent.h
	inc/MantidDataHandling/SNSDataArchive.h
	inc/MantidDataHandling/SNSLiveEventDataListener.h
	inc/MantidDataHandling/SaveAscii.h
	inc/MantidDataHandling/SaveCSV.h
	inc/MantidDataHandling/SaveCalFile.h
	inc/MantidDataHandling/SaveCanSAS1D.h
	inc/MantidDataHandling/SaveDASC.h
	inc/MantidDataHandling/SaveDaveGrp.h
	inc/MantidDataHandling/SaveDetectorMasks.h
	inc/MantidDataHandling/SaveDetectorsGrouping.h
	inc/MantidDataHandling/SaveDspacemap.h
	inc/MantidDataHandling/SaveFocusedXYE.h
	inc/MantidDataHandling/SaveGSS.h
	inc/MantidDataHandling/SaveISISNexus.h
	inc/MantidDataHandling/SaveIsawDetCal.h
	inc/MantidDataHandling/SaveMask.h
	inc/MantidDataHandling/SaveNISTDAT.h
	inc/MantidDataHandling/SaveNXSPE.h
	inc/MantidDataHandling/SaveNexus.h
	inc/MantidDataHandling/SaveNexusProcessed.h
	inc/MantidDataHandling/SavePAR.h
	inc/MantidDataHandling/SavePHX.h
	inc/MantidDataHandling/SaveRKH.h
	inc/MantidDataHandling/SaveSPE.h
	inc/MantidDataHandling/SaveToSNSHistogramNexus.h
	inc/MantidDataHandling/SaveVTK.h
	inc/MantidDataHandling/SetScalingPSD.h
	inc/MantidDataHandling/StartLiveData.h
	inc/MantidDataHandling/UpdateInstrumentFromFile.h
	src/LoadRaw/byte_rel_comp.h
	src/LoadRaw/isisraw.h
	src/LoadRaw/isisraw2.h
	src/LoadRaw/item_struct.h
	src/LoadRaw/vms_convert.h
)

set ( TEST_FILES
	#Needs fixing to not rely on network. test/SNSLiveEventDataListenerTest.h
	#test/LoadDAETest.h
	#test/LoadSNSNexusTest.h # TODO has no active tests in it
	#test/LoadSassenaTest.h
	test/AppendGeometryToSNSNexusTest.h
	test/CompressEventsTest.h
	test/CreateSampleShapeTest.h
	test/DefineGaugeVolumeTest.h
	test/DeleteTableRowsTest.h
	test/DetermineChunkingTest.h
	test/FakeEventDataListenerTest.h
	test/FileEventDataListenerTest.h
	test/FindDetectorsInShapeTest.h
	test/FindDetectorsParTest.h
	test/GenerateGroupingPowderTest.h
	test/GetMaskedDetectorsTest.h
	test/GroupDetectors2Test.h
	test/GroupDetectorsTest.h
	test/ISISDataArchiveTest.h
	test/ISISHistoDataListenerTest.h
	test/InstrumentRayTracerTest.h
	test/LiveDataAlgorithmTest.h
	test/LoadAsciiTest.h
	test/LoadCalFileTest.h
	test/LoadCanSAS1dTest.h
	test/LoadDaveGrpTest.h
	test/LoadDetectorInfoTest.h
	test/LoadDetectorsGroupingFileTest.h
	test/LoadDspacemapTest.h
	test/LoadEmptyInstrumentTest.h
	test/LoadEventNexusTest.h
	test/LoadEventPreNexus2Test.h
	test/LoadEventPreNexusTest.h
	#test/LoadILLTest.h
	test/LoadISISNexusTest.h
	test/LoadInstrumentFromRawTest.h
	test/LoadInstrumentTest.h
	test/LoadIsawDetCalTest.h
	test/LoadLiveDataTest.h
	test/LoadLogTest.h
	test/LoadLogsFromSNSNexusTest.h
	test/LoadMappingTableTest.h
	test/LoadMaskTest.h
	test/LoadMuonLogTest.h
	test/LoadMuonNexus2Test.h
	test/LoadMuonNexusTest.h
	test/LoadNXSPETest.h
	test/LoadNexusLogsTest.h
	test/LoadNexusMonitorsTest.h
	test/LoadNexusProcessedTest.h
	test/LoadNexusTest.h
	test/LoadParameterFileTest.h
	test/LoadPreNexusMonitorsTest.h
	test/LoadPreNexusTest.h
	test/LoadQKKTest.h
	test/LoadRKHTest.h
	test/LoadRSaveNLoadNcspTest.h
	test/LoadRaw2Test.h
	test/LoadRaw3Test.h
	test/LoadRawBin0Test.h
	test/LoadRawSpectrum0Test.h
	test/LoadRawTest.h
	test/LoadSNSEventNexusTest.h
	test/LoadSNSspecTest.h
	test/LoadSPETest.h
	test/LoadSaveAsciiTest.h
	test/LoadSpice2dTest.h
	test/LoadTOFRawNexusTest.h
	test/LoadTest.h
	test/ManagedRawFileWorkspace2DTest.h
	test/MaskDetectorsInShapeTest.h
	test/MaskDetectorsTest.h
	test/Merge2WorkspaceLogsTest.h
	test/MonitorLiveDataTest.h
	test/MoveInstrumentComponentTest.h
	test/NexusTesterTest.h
	test/OrbiterDataArchiveTest.h
	test/ProcessDasNexusLogTest.h
	test/RawFileInfoTest.h
	test/RemoveLogsTest.h
	test/RenameLogTest.h
	test/SNSDataArchiveTest.h
	test/SaveCSVTest.h
	test/SaveCalFileTest.h
	test/SaveCanSAS1dTest.h
	test/SaveDASCTest.h
	test/SaveDaveGrpTest.h
	test/SaveDetectorsGroupingTest.h
	test/SaveDspacemapTest.h
	test/SaveFocussedXYETest.h
	test/SaveIsawDetCalTest.h
	test/SaveMaskTest.h
	test/SaveNISTDATTest.h
	test/SaveNXSPETest.h
	test/SaveNexusProcessedTest.h
	test/SaveNexusTest.h
	test/SavePARTest.h
	test/SavePHXTest.h
	test/SaveRKHTest.h
	test/SaveSPETest.h
	test/SaveToSNSHistogramNexusTest.h
	test/SetScalingPSDTest.h
	test/StartLiveDataTest.h
	test/UpdateInstrumentFromFileTest.h
	test/XMLlogfileTest.h
)


if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(CurveFitting SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidDataHandling/PrecompiledHeader.h MantidDataHandling src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
# Add the target for this directory
add_library ( DataHandling ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( DataHandling PROPERTIES OUTPUT_NAME MantidDataHandling )
# Add to the 'Framework' group in VS
set_property ( TARGET DataHandling PROPERTY FOLDER "MantidFramework" )

#IF (${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#	SET (HDF5_DIR "${CMAKE_MODULE_PATH}")
#	SET (HDF5_CONFIG "${CMAKE_MODULE_PATH}/hdf5-config.cmake" )
#	find_package ( HDF5 COMPONENTS HL REQUIRED
#				        CONFIGS hdf5-config.cmake )
#ELSE()
#	find_package ( HDF5 COMPONENTS HL REQUIRED )
#ENDIF()

#include_directories ( inc ../Nexus/inc ${HDF5_INCLUDE_DIRS})
include_directories ( inc ../Nexus/inc)

#target_link_libraries ( DataHandling ${MANTIDLIBS} Nexus ${HDF5_LIBRARIES})
target_link_libraries ( DataHandling ${MANTIDLIBS} Nexus)

if ( CXXTEST_FOUND )
  include_directories ( ../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../TestHelpers/src/ComponentCreationHelper.cpp
                        ../TestHelpers/src/WorkspaceCreationHelper.cpp
                        test/TestDataListener.cpp 
                        )
  cxxtest_add_test ( DataHandlingTest ${TEST_FILES} )
  target_link_libraries( DataHandlingTest DataHandling )
  add_dependencies ( FrameworkTests DataHandlingTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET DataHandlingTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS DataHandling ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
