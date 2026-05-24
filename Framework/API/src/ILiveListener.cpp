// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+

#include "MantidAPI/ILiveListener.h"

namespace Mantid::API {

/** Default backward-compatible implementation of the deprecated runStatus().
 *
 *  Returns lastTransition() if a run-state edge was committed by the most
 *  recent extractData() call, otherwise returns runState().  This reproduces
 *  the historical contract for MonitorLiveData callers that call extractData()
 *  first and then poll runStatus().
 *
 *  Concrete listeners that previously overrode runStatus() with side effects
 *  must migrate those side effects to onBeginRun() / onEndRun() (SNS) or
 *  onBeforeExtract() (other listeners); see the v3 refactoring spec.
 */
ILiveListener::RunStatus ILiveListener::runStatus() {
  if (auto edge = lastTransition())
    return *edge;
  return runState();
}

} // namespace Mantid::API
