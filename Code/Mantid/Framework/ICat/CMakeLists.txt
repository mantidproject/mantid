set ( SRC_FILES
	src/CatalogDownloadDataFile.cpp
	src/CatalogGetDataFiles.cpp
	src/CatalogGetDataSets.cpp
	src/CatalogListInstruments.cpp
	src/CatalogListInvestigationTypes.cpp
	src/CatalogLogin.cpp
	src/CatalogLogout.cpp
	src/CatalogMyDataSearch.cpp
	src/CatalogSearch.cpp
	src/CatalogSearchParam.cpp
	src/ErrorHandling.cpp
	src/GSoap.cpp
	src/GSoapGenerated.cpp
	src/ICat3Catalog.cpp
	src/ICatHelper.cpp
)

set ( SRC_UNITY_IGNORE_FILES )

set ( INC_FILES
	inc/MantidICat/CatalogDownloadDataFile.h
	inc/MantidICat/CatalogGetDataFiles.h
	inc/MantidICat/CatalogGetDataSets.h
	inc/MantidICat/CatalogListInstruments.h
	inc/MantidICat/CatalogListInvestigationTypes.h
	inc/MantidICat/CatalogLogin.h
	inc/MantidICat/CatalogLogout.h
	inc/MantidICat/CatalogMyDataSearch.h
	inc/MantidICat/CatalogSearch.h
	inc/MantidICat/CatalogSearchParam.h
	inc/MantidICat/ErrorHandling.h
	inc/MantidICat/GSoap/stdsoap2.h
	inc/MantidICat/GSoapGenerated/soapH.h
	inc/MantidICat/GSoapGenerated/soapICATPortBindingProxy.h
	inc/MantidICat/GSoapGenerated/soapStub.h
	inc/MantidICat/ICat3Catalog.h
	inc/MantidICat/ICatExport.h
	inc/MantidICat/ICatHelper.h
	inc/MantidICat/Session.h
)

set ( TEST_FILES
	test/CatalogDownloadDataFileTest.h
	test/CatalogGetDataFilesTest.h
	test/CatalogGetDataSetsTest.h
	test/CatalogListInstrumentsTest.h
	test/CatalogListInvestigationTypesTest.h
	test/CatalogLoginTest.h
	test/CatalogMyDataSearchTest.h
	test/CatalogSearchTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(ICat SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add ssl dependency
find_package ( OpenSSL REQUIRED )
include_directories ( ${OPENSSL_INCLUDE_DIR} )
add_definitions ( -DWITH_OPENSSL -DWITH_NONAMESPACES )

# Add a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidICat/PrecompiledHeader.h MantidICat src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
# Add the target for this directory
add_library ( ICat ${SRC_FILES} ${INC_FILES})
add_cppcheck( ICat src/GSoapGenerated.cpp src/GSoapGenerated)
# Set the name of the generated library
set_target_properties ( ICat PROPERTIES OUTPUT_NAME MantidICat 
                                        COMPILE_DEFINITIONS IN_MANTID_ICAT )
# Add to the 'Framework' group in VS
set_property ( TARGET ICat PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( ICat ${MANTIDLIBS} ${OPENSSL_LIBRARIES} )

if ( CXXTEST_FOUND )
  # Test Helper Library 
  add_library ( ICatTestHelper "test/ICatTestHelper.cpp")
  set_target_properties ( ICatTestHelper PROPERTIES COMPILE_DEFINITIONS IN_MANTID_ICAT )
  target_link_libraries( ICatTestHelper ICat  )
  
  # The actual test suite
  cxxtest_add_test ( ICatTest ${TEST_FILES} )
  target_link_libraries( ICatTest ICat ICatTestHelper )
  add_dependencies ( FrameworkTests ICatTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET ICatTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS ICat DESTINATION ${PLUGINS_DIR} )
