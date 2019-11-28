// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARAVIEW_TIME_TO_TIMESTEP
#define MANTID_PARAVIEW_TIME_TO_TIMESTEP

#include "MantidKernel/System.h"

/** Unary operation applying visualisation platforms specific conversion from a
 time to a timestep understood by underlying mantid code, where time is treated
 as an index
 * in a single dimensional array. See MDWorkspace/MDImage.

 @author Owen Arnold, Tessella plc
 @date 14/03/2011
 */

namespace Mantid {
namespace VATES {
class DLLExport TimeToTimeStep {
private:
  // Minimum time.
  double m_timeMin{0.0};
  // Maximum time
  double m_timeMax{0.0};
  // Used for internal linear calculations.
  double m_c{0.0};
  // Used for internal linear calculations.
  double m_fraction{0.0};
  bool m_runnable{false};

  /// Constructor only accessible via 'construct' static member function.
  TimeToTimeStep(double timeMin, double timeMax, size_t nIntervalSteps);

public:
  /// Constructional method.
  static TimeToTimeStep construct(double timeMin, double timeMax,
                                  size_t nIntervalSteps);

  TimeToTimeStep() = default;

  size_t operator()(double time) const;
};
} // namespace VATES
} // namespace Mantid

#endif
