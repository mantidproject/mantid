set ( SRC_FILES
	src/Extract.cpp
	src/Group.cpp
	src/IndexInfo.cpp
	src/LegacyConversion.cpp
	src/Partitioner.cpp
	src/RoundRobinPartitioner.cpp
	src/Scatter.cpp
	src/SpectrumNumberTranslator.cpp
)

set ( INC_FILES
	inc/MantidIndexing/Conversion.h
	inc/MantidIndexing/DetectorID.h
	inc/MantidIndexing/DllConfig.h
	inc/MantidIndexing/Extract.h
	inc/MantidIndexing/GlobalSpectrumIndex.h
	inc/MantidIndexing/Group.h
	inc/MantidIndexing/IndexInfo.h
	inc/MantidIndexing/IndexSet.h
	inc/MantidIndexing/IndexType.h
	inc/MantidIndexing/LegacyConversion.h
	inc/MantidIndexing/PartitionIndex.h
	inc/MantidIndexing/Partitioner.h
	inc/MantidIndexing/RoundRobinPartitioner.h
	inc/MantidIndexing/Scatter.h
	inc/MantidIndexing/SpectrumIndexSet.h
	inc/MantidIndexing/SpectrumNumber.h
	inc/MantidIndexing/SpectrumNumberTranslator.h
)

set ( TEST_FILES
	ConversionTest.h
	DetectorIDTest.h
	ExtractTest.h
	GlobalSpectrumIndexTest.h
	GroupTest.h
	IndexInfoTest.h
	IndexSetTest.h
	IndexTypeTest.h
	LegacyConversionTest.h
	PartitionIndexTest.h
	PartitionerTest.h
	RoundRobinPartitionerTest.h
	ScatterTest.h
	SpectrumIndexSetTest.h
	SpectrumNumberTest.h
	SpectrumNumberTranslatorTest.h
)

if (COVERALLS)
  foreach( loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Indexing SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( Indexing ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( Indexing PROPERTIES OUTPUT_NAME MantidIndexing
  COMPILE_DEFINITIONS IN_MANTID_INDEXING )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(Indexing PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  set_target_properties(Indexing PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif ()

# Add to the 'Framework' group in VS
set_property ( TARGET Indexing PROPERTY FOLDER "MantidFramework" )

target_include_directories ( Indexing SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries ( Indexing LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} ${MANTIDLIBS}
  Parallel
  )

if( "${UNIX_CODENAME}" STREQUAL "trusty" )
  # Special linking options are required for some older Ubuntu gcc builds to use std::thread, specific options found here:
  # http://stackoverflow.com/questions/8649828/what-is-the-correct-link-options-to-use-stdthread-in-gcc-under-linux#comment35723468_8649908
  target_link_libraries ( Indexing LINK_PRIVATE -Wl,--no-as-needed -pthread )
endif()

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

mtd_install_targets( TARGETS Indexing INSTALL_DIRS ${LIB_DIR} ${WORKBENCH_LIB_DIR})
