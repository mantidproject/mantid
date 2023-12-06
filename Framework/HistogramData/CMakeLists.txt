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
)

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
    inc/MantidHistogramData/StandardDeviationVectorOf.h
    inc/MantidHistogramData/Validation.h
    inc/MantidHistogramData/VarianceVectorOf.h
    inc/MantidHistogramData/VectorOf.h
    inc/MantidHistogramData/XValidation.h
    inc/MantidHistogramData/YValidation.h
)

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
    StandardDeviationVectorOfTest.h
    VarianceVectorOfTest.h
    VectorOfTest.h
    XValidationTest.h
    YValidationTest.h
)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(HistogramData ${SRC_FILES} ${INC_FILES})
add_library(Mantid::HistogramData ALIAS HistogramData)

# Set the name of the generated library
set_target_properties(
  HistogramData PROPERTIES OUTPUT_NAME MantidHistogramData COMPILE_DEFINITIONS IN_MANTID_HISTOGRAMDATA
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(HistogramData PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(HistogramData PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET HistogramData PROPERTY FOLDER "MantidFramework")

target_include_directories(
  HistogramData PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

target_link_libraries(HistogramData PUBLIC Kernel)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  HistogramData PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/HistogramData>
                       $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(HistogramData FALSE)

# Installation settings
set(TARGET_EXPORT_NAME "MantidHistogramDataTargets")
mtd_install_framework_lib(TARGETS HistogramData INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
