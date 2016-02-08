#include "MantidQtSliceViewer/SliceViewerFunctions.h"

/**
* Checks if a slice, defined by a  min and a max vector, takes a slice which cuts through the workspace
* @param min: the min boundary of the slice
* @param max: the max boundary of the slice
* @param dimensions: a vector of dimension objects of the workspace which will be sliced
*/
bool doesSliceCutThroughWorkspace(const Mantid::Kernel::VMD& min, const  Mantid::Kernel::VMD& max,
            const std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions) {
  auto valueBetweenMinMax = [](const Mantid::Kernel::VMD_t value, const Mantid::Kernel::VMD_t min, const Mantid::Kernel::VMD_t max) {
    return value >= min && value <= max;
  };

  int dimCounter = 0;
  auto cutsThroughWorkspace = true;

  // Check in either dimension if the the min and max values are withing the workspace boundaries
  for (const auto& dimension : dimensions) {
    const auto minDimension = static_cast<Mantid::Kernel::VMD_t>(dimension->getMinimum());
    const auto maxDimension = static_cast<Mantid::Kernel::VMD_t>(dimension->getMaximum());

    if (!valueBetweenMinMax(min[dimCounter], minDimension, maxDimension) ||
      !valueBetweenMinMax(max[dimCounter], minDimension, maxDimension)) {
      cutsThroughWorkspace = false;
      break;
    }
  }
  return cutsThroughWorkspace;
};


