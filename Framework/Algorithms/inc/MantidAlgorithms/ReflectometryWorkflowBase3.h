// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE3_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE3_H_

#include "ReflectometryWorkflowBase2.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
namespace Mantid {
namespace Algorithms {

/** ReflectometryWorkflowBase3 : base class containing common implementation
 functionality usable by concrete reflectometry workflow algorithms. Version 3.
 This version is identical to version 2 but overrides the transmission
 properties to provide some additional output workspace properties.
 */
class DLLExport ReflectometryWorkflowBase3 : public ReflectometryWorkflowBase2 {
protected:
  /// Initialize transmission properties
  void initTransmissionProperties();
};
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYWORKFLOWBASE3_H_ */
