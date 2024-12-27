// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ChangePulsetime.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ChangePulsetime)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using std::size_t;

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ChangePulsetime::init() {
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input event workspace.");
  declareProperty(std::make_unique<PropertyWithValue<double>>("TimeOffset", Direction::Input),
                  "Number of seconds (a float) to add to each event's pulse "
                  "time. Required.");
  declareProperty(std::make_unique<ArrayProperty<int>>("WorkspaceIndexList", ""),
                  "An optional list of workspace indices to change. If blank, "
                  "all spectra in the workspace are modified.");
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output event workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ChangePulsetime::exec() {
  EventWorkspace_const_sptr in_ws = getProperty("InputWorkspace");
  EventWorkspace_sptr out_ws = getProperty("OutputWorkspace");
  if (!out_ws) {
    out_ws = in_ws->clone();
  }

  // Either use the given list or use all spectra
  std::vector<int> workspaceIndices = getProperty("WorkspaceIndexList");
  auto num_to_do = static_cast<int64_t>(workspaceIndices.size());
  bool doAll = false;
  if (workspaceIndices.empty()) {
    doAll = true;
    num_to_do = in_ws->getNumberHistograms();
  }

  double timeOffset = getProperty("TimeOffset");

  Progress prog(this, 0.0, 1.0, num_to_do);
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < num_to_do; i++) {
    // What workspace index?
    int64_t wi;
    if (doAll)
      wi = i;
    else
      wi = workspaceIndices[i];

    // Call the method on the event list
    out_ws->getSpectrum(wi).addPulsetime(timeOffset);

    prog.report(name());
  }

  setProperty("OutputWorkspace", out_ws);
}

} // namespace Mantid::Algorithms
