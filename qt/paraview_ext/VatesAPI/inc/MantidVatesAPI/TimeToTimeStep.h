// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARAVIEW_TIME_TO_TIMESTEP
#define MANTID_PARAVIEW_TIME_TO_TIMESTEP

#include "MantidKernel/System.h"
#include <functional>

/** Unary operation applying visualisation platforms specific conversion from a
 time to a timestep understood by underlying mantid code, where time is treated
 as an index
 * in a single dimensional array. See MDWorkspace/MDImage.

 @author Owen Arnold, Tessella plc
 @date 14/03/2011
 */

namespace Mantid {
namespace VATES {
class DLLExport TimeToTimeStep : std::unary_function<double, int> {
private:
  // Minimum time.
  double m_timeMin;
  // Maximum time
  double m_timeMax;
  // Used for internal linear calculations.
  double m_c;
  // Used for internal linear calculations.
  double m_fraction;
  bool m_runnable;

  /// Constructor only accessible via 'construct' static member function.
  TimeToTimeStep(double timeMin, double timeMax, size_t nIntervalSteps);

public:
  /// Constructional method.
  static TimeToTimeStep construct(double timeMin, double timeMax,
                                  size_t nIntervalSteps);

  TimeToTimeStep();

  size_t operator()(double time) const;
};
} // namespace VATES
} // namespace Mantid

#endif
