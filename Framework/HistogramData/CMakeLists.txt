set ( SRC_FILES
  src/BinEdges.cpp
  src/Histogram.cpp
  src/HistogramX.cpp
  src/Points.cpp
  )

set ( INC_FILES
	inc/MantidHistogramData/BinEdges.h
	inc/MantidHistogramData/FixedLengthVector.h
	inc/MantidHistogramData/Histogram.h
	inc/MantidHistogramData/HistogramDx.h
	inc/MantidHistogramData/HistogramX.h
	inc/MantidHistogramData/Iterable.h
	inc/MantidHistogramData/Offsetable.h
	inc/MantidHistogramData/Points.h
	inc/MantidHistogramData/Scalable.h
	inc/MantidHistogramData/VectorOf.h
)

set ( TEST_FILES
	BinEdgesTest.h
	FixedLengthVectorTest.h
	HistogramDxTest.h
	HistogramTest.h
	HistogramXTest.h
	IterableTest.h
	OffsetableTest.h
	PointsTest.h
	ScalableTest.h
	VectorOfTest.h
)

if (COVERALLS)
  foreach( loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(HistogramData SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( HistogramData ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( HistogramData PROPERTIES OUTPUT_NAME MantidHistogramData
  COMPILE_DEFINITIONS IN_MANTID_HISTOGRAMDATA )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( HistogramData PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

# Add to the 'Framework' group in VS
set_property ( TARGET HistogramData PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( HistogramData LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} ${MANTIDLIBS} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS HistogramData ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
