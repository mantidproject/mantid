#ifndef SLICE_VIEWER_SLICE_VIEWER_FUNCTIONS_TEST_H_
#define SLICE_VIEWER_SLICE_VIEWER_FUNCTIONS_TEST_H_

#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidQtWidgets/SliceViewer/SliceViewerFunctions.h"
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::SliceViewer;

namespace {

struct SliceDefinition {
  SliceDefinition(size_t numDims) : min(numDims), max(numDims) {}
  Mantid::Kernel::VMD min;
  Mantid::Kernel::VMD max;
};

SliceDefinition get_slice_definition(const size_t numberOfDimensions,
                                     const Mantid::Kernel::VMD_t minValue,
                                     const Mantid::Kernel::VMD_t maxValue) {
  SliceDefinition sliceDefinition(numberOfDimensions);
  Mantid::Kernel::VMD min(numberOfDimensions);
  Mantid::Kernel::VMD max(numberOfDimensions);

  for (size_t index = 0; index < numberOfDimensions; ++index) {
    sliceDefinition.min[index] = minValue;
    sliceDefinition.max[index] = maxValue;
  }
  return sliceDefinition;
}

std::vector<Mantid::Geometry::MDHistoDimension_sptr> get_dimensions_collection(
    const size_t numberOfDimensions, const Mantid::Kernel::VMD_t minValue,
    const Mantid::Kernel::VMD_t maxValue, const std::string sliceLies) {

  std::vector<Mantid::Geometry::MDHistoDimension_sptr> dimensions(
      numberOfDimensions);

  const size_t numberOfBins = 5;
  auto minConverted = static_cast<Mantid::coord_t>(minValue);
  auto maxConverted = static_cast<Mantid::coord_t>(maxValue);

  const Mantid::coord_t shift = 0.5;

  if (sliceLies == "inside") {
    minConverted = minConverted - shift;
    maxConverted = maxConverted + shift;
  } else if (sliceLies == "outside") {
    minConverted = minConverted + shift;
    maxConverted = maxConverted - shift;
  } else {
    minConverted = minConverted + shift;
    maxConverted = maxConverted + shift;
  }

  for (size_t index = 0; index < numberOfDimensions; ++index) {
    Mantid::Kernel::UnitLabel unitLabel("Meters");
    Mantid::Geometry::GeneralFrame frame("Length", unitLabel);
    auto dimension = boost::make_shared<Mantid::Geometry::MDHistoDimension>(
        "Distance", "Dist", frame, minConverted, maxConverted, numberOfBins);
    dimensions[index] = dimension;
  }

  return dimensions;
}
} // namespace

class SliceViewerFunctionsTest : public CxxTest::TestSuite {
public:
  bool do_test_slice_lies_in_workspace_boundaries(
      const std::string sliceLiesWithinWorkspaceBoundary) {
    // Arrange
    const size_t numberOfDimensions = 3;
    Mantid::Kernel::VMD_t minValue = 1;
    Mantid::Kernel::VMD_t maxValue = 3;

    auto sliceDefinition =
        get_slice_definition(numberOfDimensions, minValue, maxValue);
    auto dimensions =
        get_dimensions_collection(numberOfDimensions, minValue, maxValue,
                                  sliceLiesWithinWorkspaceBoundary);

    // Act
    return doesSliceCutThroughWorkspace(sliceDefinition.min,
                                        sliceDefinition.max, dimensions);
  }

  void test_that_finds_slice_point_within_workspace_boundaries() {
    std::string sliceLiesWithinWorkspaceBoundary = "inside";
    auto liesInside = do_test_slice_lies_in_workspace_boundaries(
        sliceLiesWithinWorkspaceBoundary);
    TSM_ASSERT("Slice definition should lie within the workspace boundary",
               liesInside);
  }

  void test_that_finds_slice_point_outside_workspace_boudaries() {
    std::string sliceLiesWithinWorkspaceBoundary = "outside";
    auto liesInside = do_test_slice_lies_in_workspace_boundaries(
        sliceLiesWithinWorkspaceBoundary);
    TSM_ASSERT("Slice definition should not lie within the workspace boundary",
               !liesInside);
  }

  void
  test_that_slice_point_which_is_partially_in_the_workspace_is_detected_as_not_being_in_the_workspace() {
    std::string sliceLiesWithinWorkspaceBoundary = "partially";
    auto liesInside = do_test_slice_lies_in_workspace_boundaries(
        sliceLiesWithinWorkspaceBoundary);
    TSM_ASSERT("Slice definition should lie within the workspace boundary",
               liesInside);
  }
};

#endif

// end SLICE_VIEWER_SLICE_VIEWER_FUNCTIONS_TEST_H_
