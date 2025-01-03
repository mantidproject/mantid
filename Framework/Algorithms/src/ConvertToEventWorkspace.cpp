// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConvertToEventWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include <limits>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToEventWorkspace)

/** Initialize the algorithm's properties.
 */
void ConvertToEventWorkspace::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace2D>>("InputWorkspace", "", Direction::Input),
                  "An input Workspace2D.");
  declareProperty("GenerateZeros", false,
                  "Generate an event even for empty bins\n"
                  "Warning! This may use significantly more memory.");
  declareProperty("GenerateMultipleEvents", false,
                  "Generate a number of evenly spread events in each bin. See "
                  "the help for details.\n"
                  "Warning! This may use significantly more memory.");
  declareProperty("MaxEventsPerBin", 10,
                  "If GenerateMultipleEvents is true, specifies a maximum number of events "
                  "to generate in a single bin.\n"
                  "Use a value that matches your instrument's TOF resolution. Default 10.");
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output EventWorkspace.");
}

/** Execute the algorithm.
 */
void ConvertToEventWorkspace::exec() {
  Workspace2D_const_sptr inWS = getProperty("InputWorkspace");

  bool GenerateMultipleEvents = getProperty("GenerateMultipleEvents");
  bool GenerateZeros = getProperty("GenerateZeros");
  int MaxEventsPerBin = getProperty("MaxEventsPerBin");

  auto outWS = create<EventWorkspace>(*inWS);

  Progress prog(this, 0.0, 1.0, inWS->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*inWS))
  for (int iwi = 0; iwi < int(inWS->getNumberHistograms()); iwi++) {
    PARALLEL_START_INTERRUPT_REGION
    auto wi = size_t(iwi);

    // The input spectrum (a histogram)
    const auto &inSpec = inWS->getSpectrum(wi);

    // The output event list
    EventList &el = outWS->getSpectrum(wi);

    // This method fills in the events
    el.createFromHistogram(&inSpec, GenerateZeros, GenerateMultipleEvents, MaxEventsPerBin);

    prog.report("Converting");
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // Set the output
  setProperty("OutputWorkspace", std::move(outWS));
}

} // namespace Mantid::Algorithms
