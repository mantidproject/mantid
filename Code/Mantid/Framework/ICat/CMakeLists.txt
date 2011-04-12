set ( SRC_FILES src/DownloadDataFile.cpp
                src/ErrorHandling.cpp
		src/GetDataFiles.cpp
		src/GetDataSets.cpp
		src/GSoapGenerated/soapC.cpp
		src/GSoapGenerated/soapICATPortBindingProxy.cpp
		src/GSoap/stdsoap2.cpp
		src/ICat3Catalog.cpp
		src/ICatHelper.cpp
		src/ListInstruments.cpp
		src/ListInvestigationTypes.cpp
		src/Login.cpp
		src/Logout.cpp
		src/MyDataSearch.cpp
		src/Search.cpp
		src/SearchParam.cpp )

set ( SRC_UNITY_IGNORE_FILES )

set ( INC_FILES  inc/MantidICat/DownloadDataFile.h
				 inc/MantidICat/ErrorHandling.h
		inc/MantidICat/GetDataFiles.h
		inc/MantidICat/GetDataSets.h
		inc/MantidICat/GSoapGenerated/soapH.h
		inc/MantidICat/GSoapGenerated/soapICATPortBindingProxy.h
		inc/MantidICat/GSoapGenerated/soapStub.h
		inc/MantidICat/GSoap/stdsoap2.h
		inc/MantidICat/ICat3Catalog.h
		inc/MantidICat/ICatExport.h
		inc/MantidICat/ICatHelper.h
		inc/MantidICat/ListInstruments.h
		inc/MantidICat/ListInvestigationTypes.h
		inc/MantidICat/Login.h
		inc/MantidICat/Logout.h
		inc/MantidICat/MyDataSearch.h
		inc/MantidICat/Search.h
		inc/MantidICat/SearchParam.h
		inc/MantidICat/Session.h )

set ( TEST_FILES test/DownloadDataFileTest.h
				 test/GetDataFilesTest.h
		 test/GetDataSetsTest.h
		 test/ListInstrumentsTest.h
		 test/ListInvestigationTypesTest.h
		 test/LoginTest.h
		 test/MyDataSearchTest.h
		 test/SearchTest.h )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(ICat SRC_FILES SRC_UNITY_IGNORE_FILES 1000)
endif(UNITY_BUILD)

# Add ssl dependency
find_package ( OpenSSL REQUIRED )
include_directories ( ${OPENSSL_INCLUDE_DIR} )
add_definitions ( -DWITH_OPENSSL -DWITH_NONAMESPACES )

# Add the target for this directory
add_library ( ICat ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( ICat PROPERTIES OUTPUT_NAME MantidICat 
                                        COMPILE_DEFINITIONS IN_MANTID_ICAT )
# Add to the 'Framework' group in VS
set_property ( TARGET ICat PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( ICat ${MANTIDLIBS} ${OPENSSL_LIBRARIES} )

if ( CXXTEST_FOUND )
  cxxtest_add_test ( ICatTest ${TEST_FILES} )
  target_link_libraries( ICatTest ICat )
  add_dependencies ( FrameworkTests ICatTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET ICatTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS ICat DESTINATION ${PLUGINS_DIR} )
