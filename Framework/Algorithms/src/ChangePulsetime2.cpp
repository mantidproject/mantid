// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ChangePulsetime2.h"
#include "MantidAPI/Algorithm.tcc"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid::Algorithms {

// Algorithm not registered as this will be used for testing MPI
// DECLARE_ALGORITHM(ChangePulsetime2)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using std::size_t;

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ChangePulsetime2::init() {
  declareWorkspaceInputProperties<EventWorkspace>("InputWorkspace", "An input event workspace.");
  declareProperty(std::make_unique<PropertyWithValue<double>>("TimeOffset", Direction::Input),
                  "Number of seconds (a float) to add to each event's pulse "
                  "time. Required.");
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output event workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ChangePulsetime2::exec() {
  EventWorkspace_const_sptr in_ws;
  Indexing::SpectrumIndexSet indexSet;

  std::tie(in_ws, indexSet) = getWorkspaceAndIndices<EventWorkspace>("InputWorkspace");
  EventWorkspace_sptr out_ws = getProperty("OutputWorkspace");
  if (!out_ws) {
    out_ws = in_ws->clone();
  }

  // Either use the given list or use all spectra
  double timeOffset = getProperty("TimeOffset");

  Progress prog(this, 0.0, 1.0, indexSet.size());
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < static_cast<int64_t>(indexSet.size()); i++) {
    // What workspace index?

    // Call the method on the event list
    out_ws->getSpectrum(indexSet[i]).addPulsetime(timeOffset);
    prog.report(name());
  }

  setProperty("OutputWorkspace", out_ws);
}

} // namespace Mantid::Algorithms
