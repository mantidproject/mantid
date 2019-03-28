set(SRC_FILES
    src/BinEdges.cpp
    src/CountStandardDeviations.cpp
    src/CountVariances.cpp
    src/Counts.cpp
    src/EstimatePolynomial.cpp
    src/Exception.cpp
    src/Frequencies.cpp
    src/FrequencyStandardDeviations.cpp
    src/FrequencyVariances.cpp
    src/Histogram.cpp
    src/HistogramBuilder.cpp
    src/HistogramMath.cpp
    src/Interpolate.cpp
    src/Points.cpp
    src/Rebin.cpp
    src/Slice.cpp)

set(INC_FILES
    inc/MantidHistogramData/Addable.h
    inc/MantidHistogramData/BinEdges.h
    inc/MantidHistogramData/CountStandardDeviations.h
    inc/MantidHistogramData/CountVariances.h
    inc/MantidHistogramData/Counts.h
    inc/MantidHistogramData/EValidation.h
    inc/MantidHistogramData/EstimatePolynomial.h
    inc/MantidHistogramData/Exception.h
    inc/MantidHistogramData/FixedLengthVector.h
    inc/MantidHistogramData/Frequencies.h
    inc/MantidHistogramData/FrequencyStandardDeviations.h
    inc/MantidHistogramData/FrequencyVariances.h
    inc/MantidHistogramData/Histogram.h
    inc/MantidHistogramData/HistogramBuilder.h
    inc/MantidHistogramData/HistogramDx.h
    inc/MantidHistogramData/HistogramE.h
    inc/MantidHistogramData/HistogramItem.h
    inc/MantidHistogramData/HistogramIterator.h
    inc/MantidHistogramData/HistogramMath.h
    inc/MantidHistogramData/HistogramX.h
    inc/MantidHistogramData/HistogramY.h
    inc/MantidHistogramData/Interpolate.h
    inc/MantidHistogramData/Iterable.h
    inc/MantidHistogramData/LinearGenerator.h
    inc/MantidHistogramData/LogarithmicGenerator.h
    inc/MantidHistogramData/Multipliable.h
    inc/MantidHistogramData/Offsetable.h
    inc/MantidHistogramData/PointStandardDeviations.h
    inc/MantidHistogramData/PointVariances.h
    inc/MantidHistogramData/Points.h
    inc/MantidHistogramData/QuadraticGenerator.h
    inc/MantidHistogramData/Rebin.h
    inc/MantidHistogramData/Scalable.h
    inc/MantidHistogramData/Slice.h
    inc/MantidHistogramData/StandardDeviationVectorOf.h
    inc/MantidHistogramData/Validation.h
    inc/MantidHistogramData/VarianceVectorOf.h
    inc/MantidHistogramData/VectorOf.h
    inc/MantidHistogramData/XValidation.h
    inc/MantidHistogramData/YValidation.h)

set(TEST_FILES
    AddableTest.h
    BinEdgesTest.h
    CountStandardDeviationsTest.h
    CountVariancesTest.h
    CountsTest.h
    EValidationTest.h
    EstimatePolynomialTest.h
    FixedLengthVectorTest.h
    FrequenciesTest.h
    FrequencyStandardDeviationsTest.h
    FrequencyVariancesTest.h
    HistogramBuilderTest.h
    HistogramDxTest.h
    HistogramETest.h
    HistogramIteratorTest.h
    HistogramMathTest.h
    HistogramTest.h
    HistogramXTest.h
    HistogramYTest.h
    InterpolateTest.h
    IterableTest.h
    LinearGeneratorTest.h
    LogarithmicGeneratorTest.h
    MultipliableTest.h
    OffsetableTest.h
    PointStandardDeviationsTest.h
    PointVariancesTest.h
    PointsTest.h
    QuadraticGeneratorTest.h
    RebinTest.h
    ScalableTest.h
    SliceTest.h
    StandardDeviationVectorOfTest.h
    VarianceVectorOfTest.h
    VectorOfTest.h
    XValidationTest.h
    YValidationTest.h)

if(COVERALLS)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(HistogramData SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library(HistogramData ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties(HistogramData
                      PROPERTIES OUTPUT_NAME
                                 MantidHistogramData
                                 COMPILE_DEFINITIONS
                                 IN_MANTID_HISTOGRAMDATA)

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(HistogramData
                        PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(HistogramData
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET HistogramData PROPERTY FOLDER "MantidFramework")

target_include_directories(HistogramData PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(HistogramData
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${GSL_LIBRARIES}
                      ${MANTIDLIBS})

# Add the unit tests directory
add_subdirectory(test)

# Installation settings

mtd_install_targets(TARGETS
                    HistogramData
                    INSTALL_DIRS
                    ${LIB_DIR}
                    ${WORKBENCH_LIB_DIR})
