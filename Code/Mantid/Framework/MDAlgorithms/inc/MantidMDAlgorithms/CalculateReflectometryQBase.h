#ifndef MANTID_MDALGORITHMS_CALCULATE_REFLECTOMETRYQ_BASE_H_
#define MANTID_MDALGORITHMS_CALCULATE_REFLECTOMETRYQ_BASE_H_

#include "MantidKernel/System.h"

namespace Mantid {
namespace Geometry {
namespace MDGeometry {
class IMDDimension;
}
}

namespace MDAlgorithms {
/**
Base class for reflectometry Q transformations
*/
class DLLExport CalculateReflectometryQBase {
protected:
  const double to_radians_factor;
  CalculateReflectometryQBase();

protected:
  ~CalculateReflectometryQBase();
};
}
}

#endif
