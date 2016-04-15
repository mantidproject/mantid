set ( SRC_FILES
  src/BinEdges.cpp
  src/Histogram.cpp
  src/HistogramX.cpp
  src/Points.cpp
  )

set ( INC_FILES
  inc/MantidHistogram/BinEdges.h
  inc/MantidHistogram/ConstIterable.h
  inc/MantidHistogram/Histogram.h
  inc/MantidHistogram/HistogramData.h
  inc/MantidHistogram/HistogramX.h
  inc/MantidHistogram/Points.h
  inc/MantidHistogram/VectorOf.h
  )

set ( TEST_FILES
  HistogramDataTest.h
  BinEdgesTest.h
  ConstIterableTest.h
  HistogramXTest.h
  PointsTest.h
  HistogramTest.h
  VectorOfTest.h
  )

if (COVERALLS)
  foreach( loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Histogram SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( Histogram ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties ( Histogram PROPERTIES OUTPUT_NAME MantidHistogram
  COMPILE_DEFINITIONS IN_MANTID_HISTOGRAM )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( Histogram PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

# Add to the 'Framework' group in VS
set_property ( TARGET Histogram PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( Histogram LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME} ${MANTIDLIBS} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS Histogram  ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
