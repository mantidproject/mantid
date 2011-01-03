set ( SRC_FILES src/CreateSampleShape.cpp src/DefineGaugeVolume.cpp
      src/FindDetectorsInShape.cpp src/GetMaskedDetectors.cpp
      src/GroupDetectors2.cpp src/GroupDetectors.cpp
      src/ISISDataArchive.cpp src/LoadAscii.cpp src/LoadCanSAS1D.cpp
      src/Load.cpp src/LoadDAE.cpp src/LoadDAE/idc.cpp
      src/LoadDAE/isisds_command.cpp src/LoadDetectorInfo.cpp
      src/LoadEmptyInstrument.cpp src/LoadEventPreNeXus.cpp
      src/LoadGSS.cpp src/LoadInstrument.cpp
      src/LoadInstrumentFromRaw.cpp src/LoadLog.cpp
      src/LoadInstrumentHelper.cpp
      src/LoadLOQDistancesFromRaw.cpp src/LoadMappingTable.cpp
      src/LoadParameterFile.cpp src/LoadPreNeXusMonitors.cpp
      src/LoadRaw2.cpp src/LoadRaw3.cpp src/LoadRawBin0.cpp
      src/LoadRaw/byte_rel_comp.cpp src/LoadRaw.cpp
      src/LoadRawHelper.cpp src/LoadRaw/isisraw2.cpp
      src/LoadRaw/isisraw.cpp src/LoadRaw/item_struct.cpp
      src/LoadRawSpectrum0.cpp src/LoadRaw/vms_convert.cpp
      src/LoadRKH.cpp src/LoadSampleDetailsFromRaw.cpp
      src/LoadSNSspec.cpp src/LoadSpec.cpp src/LoadSPE.cpp
      src/LoadSpice2D.cpp src/ManagedRawFileWorkspace2D.cpp
      src/MaskDetectors.cpp src/MaskDetectorsInShape.cpp
      src/MoveInstrumentComponent.cpp src/RawFileInfo.cpp
      src/RotateInstrumentComponent.cpp src/SaveAscii.cpp
      src/SaveCanSAS1D.cpp src/SaveCSV.cpp src/SaveDASC.cpp
      src/SaveFocusedXYE.cpp src/SaveGSS.cpp src/SavePHX.cpp
      src/SaveRKH.cpp src/SaveSPE.cpp src/SaveVTK.cpp
      src/SetScalingPSD.cpp src/UpdateInstrumentFromRaw.cpp 
)

set ( INC_FILES inc/MantidDataHandling/CreateSampleShape.h
      inc/MantidDataHandling/DefineGaugeVolume.h
      inc/MantidDataHandling/FindDetectorsInShape.h
      inc/MantidDataHandling/GetMaskedDetectors.h
      inc/MantidDataHandling/GroupDetectors2.h
      inc/MantidDataHandling/GroupDetectors.h
      inc/MantidDataHandling/ISISDataArchive.h
      inc/MantidDataHandling/LoadAscii.h
      inc/MantidDataHandling/LoadCanSAS1D.h
      inc/MantidDataHandling/LoadDAE.h
      inc/MantidDataHandling/LoadDetectorInfo.h
      inc/MantidDataHandling/LoadEmptyInstrument.h
      inc/MantidDataHandling/LoadEventPreNeXus.h
      inc/MantidDataHandling/LoadGSS.h inc/MantidDataHandling/Load.h
      inc/MantidDataHandling/LoadInstrumentFromRaw.h
      inc/MantidDataHandling/LoadInstrument.h
      inc/MantidDataHandling/LoadLog.h
      inc/MantidDataHandling/LoadLOQDistancesFromRaw.h
      inc/MantidDataHandling/LoadMappingTable.h
      inc/MantidDataHandling/LoadParameterFile.h
      inc/MantidDataHandling/LoadPreNeXusMonitors.h
      inc/MantidDataHandling/LoadRaw2.h
      inc/MantidDataHandling/LoadRaw3.h
      inc/MantidDataHandling/LoadRawBin0.h
      inc/MantidDataHandling/LoadRaw.h
      inc/MantidDataHandling/LoadRawHelper.h
      inc/MantidDataHandling/LoadRawSpectrum0.h
      inc/MantidDataHandling/LoadRKH.h
      inc/MantidDataHandling/LoadSampleDetailsFromRaw.h
      inc/MantidDataHandling/LoadSNSspec.h
      inc/MantidDataHandling/LoadSpec.h
      inc/MantidDataHandling/LoadSPE.h
      inc/MantidDataHandling/LoadSpice2D.h
      inc/MantidDataHandling/ManagedRawFileWorkspace2D.h
      inc/MantidDataHandling/MaskDetectors.h
      inc/MantidDataHandling/MaskDetectorsInShape.h
      inc/MantidDataHandling/MoveInstrumentComponent.h
      inc/MantidDataHandling/RawFileInfo.h
      inc/MantidDataHandling/RotateInstrumentComponent.h
      inc/MantidDataHandling/SaveAscii.h
      inc/MantidDataHandling/SaveCanSAS1D.h
      inc/MantidDataHandling/SaveCSV.h
      inc/MantidDataHandling/SaveDASC.h
      inc/MantidDataHandling/SaveFocusedXYE.h
      inc/MantidDataHandling/SaveGSS.h
      inc/MantidDataHandling/SavePHX.h
      inc/MantidDataHandling/SaveRKH.h
      inc/MantidDataHandling/SaveSPE.h
      inc/MantidDataHandling/SaveVTK.h
      inc/MantidDataHandling/SetScalingPSD.h
      inc/MantidDataHandling/UpdateInstrumentFromRaw.h 
)

# Add the target for this directory
add_library ( DataHandling ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( DataHandling PROPERTIES OUTPUT_NAME MantidDataHandling )
# Add to the 'Framework' group in VS
set_property ( TARGET DataHandling PROPERTY FOLDER "Framework" )

include_directories ( inc )

target_link_libraries ( DataHandling ${MANTIDLIBS} )
