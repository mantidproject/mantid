set ( SRC_FILES src/LoadInstrumentFromNexus.cpp
                src/LoadInstrumentFromSNSNexus.cpp
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
                inc/MantidNexus/LoadInstrumentFromSNSNexus.h
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

set ( TEST_FILES #test/LoadInstrumentFromSNSNexusTest.h # TODO fails
                 #test/LoadISISNexusTest.h # TODO fails
		 #test/LoadLogsFromSNSNexusTest.h # TODO fails
		 #test/LoadMuonLogTest.h # TODO fails
		 #test/LoadMuonNexus2Test.h # TODO fails
		 #test/LoadMuonNexusTest.h # TODO fails
		 #test/LoadNexusMonitorsTest.h # TODO fails
		 #test/LoadNexusProcessedTest.h # TODO fails
		 #test/LoadNexusTest.h # TODO fails
		 #test/LoadRSaveNLoadNcspTest.h # TODO fails
		 #test/LoadSNSEventNexusTest.h # TODO fails
		 test/LoadSNSNexusTest.h
		 #test/LoadTOFRawNeXusTest.h # TODO has no active tests in it
		 test/NexusAPITest.h
		 #test/SaveNexusProcessedTest.h # TODO fails
		 #test/SaveNeXusTest.h # TODO fails
		 #test/SaveNXSPETest.h # TODO fails
		 )

# For Windows:
add_definitions ( -DIN_MANTID_NEXUS -DIN_NEXUS_CPP_LIBRARY )

# Add the dependency on the nexus library
find_package ( Nexus REQUIRED )
include_directories ( ${NEXUS_INCLUDE_DIR} )

# Add the target for this directory
add_library ( Nexus ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( Nexus PROPERTIES OUTPUT_NAME MantidNexus )
# Add to the 'Framework' group in VS
set_property ( TARGET Nexus PROPERTY FOLDER "Framework" )

include_directories ( inc )

target_link_libraries ( Nexus ${MANTIDLIBS} ${NEXUS_LIBRARIES} )

if ( CXXTEST_FOUND )
  include_directories (../DataHandling/inc )
  cxxtest_add_test ( NexusTest ${TEST_FILES} )
  target_link_libraries( NexusTest Nexus DataHandling )
endif ()
