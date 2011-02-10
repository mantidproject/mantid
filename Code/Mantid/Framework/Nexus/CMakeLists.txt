set ( SRC_FILES src/LoadInstrumentFromNexus.cpp
		src/LoadISISNexus2.cpp
		src/LoadISISNexus.cpp
		src/LoadLogsFromSNSNexus.cpp
		src/LoadMuonLog.cpp
		src/LoadMuonNexus2.cpp
		src/LoadMuonNexus.cpp
		src/LoadNeXus.cpp
		src/LoadNexusMonitors.cpp
		src/LoadNexusProcessed.cpp
		src/LoadSNSEventNexus.cpp
		src/LoadSNSNexus.cpp
		src/LoadTOFRawNeXus.cpp
		src/MuonNexusReader.cpp
		src/NexusClasses.cpp
		src/NeXusException.cpp
		src/NeXusFile.cpp
		src/NexusFileIO.cpp
		src/NeXusStream.cpp
		src/SaveNeXus.cpp
		src/SaveNexusProcessed.cpp
		src/SaveNXSPE.cpp
		src/SaveSNSNexus.cpp )

set ( INC_FILES inc/MantidNexus/LoadInstrumentFromNexus.h
		inc/MantidNexus/LoadISISNexus2.h
		inc/MantidNexus/LoadISISNexus.h
		inc/MantidNexus/LoadLogsFromSNSNexus.h
		inc/MantidNexus/LoadMuonLog.h
		inc/MantidNexus/LoadMuonNexus2.h
		inc/MantidNexus/LoadMuonNexus.h
		inc/MantidNexus/LoadNeXus.h
		inc/MantidNexus/LoadNexusMonitors.h
		inc/MantidNexus/LoadNexusProcessed.h
		inc/MantidNexus/LoadSNSEventNexus.h
		inc/MantidNexus/LoadSNSNexus.h
		inc/MantidNexus/LoadTOFRawNeXus.h
		inc/MantidNexus/MuonNexusReader.h
		inc/MantidNexus/NexusClasses.h
		inc/MantidNexus/NexusFileIO.h
		inc/MantidNexus/SaveNeXus.h
		inc/MantidNexus/SaveNexusProcessed.h
		inc/MantidNexus/SaveNXSPE.h
		inc/MantidNexus/SaveSNSNexus.h )

set ( TEST_FILES test/LoadISISNexusTest.h
		 test/LoadLogsFromSNSNexusTest.h
		 test/LoadMuonLogTest.h
		 test/LoadMuonNexus2Test.h
		 test/LoadMuonNexusTest.h
		 test/LoadNexusMonitorsTest.h
		 test/LoadNexusProcessedTest.h
		 test/LoadNeXusTest.h
		 test/LoadRSaveNLoadNcspTest.h
		 test/LoadSNSEventNexusTest.h
		 #test/LoadSNSNexusTest.h # TODO has no active tests in it
		 #test/LoadTOFRawNeXusTest.h # TODO has no active tests in it
		 test/NexusAPITest.h
		 test/SaveNexusProcessedTest.h
		 test/SaveNeXusTest.h
		 test/SaveNXSPETest.h
		 )

# Add the dependency on the nexus library
find_package ( Nexus REQUIRED )
include_directories ( ${NEXUS_INCLUDE_DIR} )

add_definitions ( -DIN_NEXUS_CPP_LIBRARY )

# Add the target for this directory
add_library ( Nexus ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Nexus PROPERTIES OUTPUT_NAME MantidNexus 
                                         COMPILE_DEFINITIONS IN_MANTID_NEXUS )
# Add to the 'Framework' group in VS
set_property ( TARGET Nexus PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( Nexus ${MANTIDLIBS} ${NEXUS_LIBRARIES} )

if ( CXXTEST_FOUND )
  include_directories (../DataHandling/inc ../TestHelpers/inc)
  cxxtest_add_test ( NexusTest ${TEST_FILES} )
  target_link_libraries( NexusTest Nexus DataHandling TestHelpers)
  add_dependencies ( FrameworkTests NexusTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET NexusTest PROPERTY FOLDER "UnitTests" )
endif ()
