// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARAVIEW_TIMESTEP_TO_TIMESTEP
#define MANTID_PARAVIEW_TIMESTEP_TO_TIMESTEP

#include "MantidKernel/System.h"
#include <functional>

/** Maps from a timestep to a timestep. Provides the static compile time
 polymorphism required by vtkDataSetFactory type classes.

 @author Owen Arnold, Tessella plc
 @date 14/03/2011
 */

namespace Mantid {
namespace VATES {
class DLLExport TimeStepToTimeStep : std::unary_function<int, int> {
private:
  TimeStepToTimeStep(double timeMin, double timeMax, size_t intervalStep);

public:
  /// Constructional method.
  static TimeStepToTimeStep construct(double timeMin, double timeMax,
                                      size_t nIntervalSteps);

  TimeStepToTimeStep() = default;

  size_t operator()(double timeStep) const;
};
} // namespace VATES
} // namespace Mantid

#endif
