// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/UnaryOperation.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/EventWorkspace.h"

#include "MantidDataObjects/RebinnedOutput.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid::Algorithms {
/** Initialisation method.
 *  Defines input and output workspace properties
 */
void UnaryOperation::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(inputPropName(), "", Direction::Input),
                  "The name of the input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(outputPropName(), "", Direction::Output),
                  "The name to use for the output workspace (can be the same "
                  "as the input one).");

  // Call the virtual defineProperties functions to declare any properties
  // defined in concrete algorithm
  defineProperties();
}

/// Executes the algorithm
void UnaryOperation::exec() {
  // get the input workspaces
  MatrixWorkspace_const_sptr in_work = getProperty(inputPropName());

  // Check if it is an event workspace
  EventWorkspace_const_sptr eventW = std::dynamic_pointer_cast<const EventWorkspace>(in_work);
  if ((eventW != nullptr) && !(this->useHistogram)) {
    this->execEvent();
    return;
  }

  MatrixWorkspace_sptr out_work = getProperty(outputPropName());
  // Only create output workspace if different to input one
  if (out_work != in_work) {
    if (in_work->id() == "EventWorkspace") {
      // Handles case of EventList which needs to be converted to Workspace2D
      out_work = WorkspaceFactory::Instance().create(in_work);
    } else {
      out_work = in_work->clone();
    }
    setProperty(outputPropName(), out_work);
  }

  // Now fetch any properties defined by concrete algorithm
  retrieveProperties();

  const size_t numSpec = in_work->getNumberHistograms();
  const size_t specSize = in_work->blocksize();

  // Initialise the progress reporting object
  Progress progress(this, 0.0, 1.0, numSpec);

  // Loop over every cell in the workspace, calling the abstract correction
  // function
  PARALLEL_FOR_IF(Kernel::threadSafe(*in_work, *out_work))
  for (int64_t i = 0; i < int64_t(numSpec); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    // Copy the X values over
    out_work->setSharedX(i, in_work->sharedX(i));
    // Get references to the data
    // Output (non-const) ones first because they may copy the vector
    // if it's shared, which isn't thread-safe.
    auto &YOut = out_work->mutableY(i);
    auto &EOut = out_work->mutableE(i);
    const auto X = in_work->points(i);
    const auto &Y = in_work->y(i);
    const auto &E = in_work->e(i);

    for (size_t j = 0; j < specSize; ++j) {
      // Call the abstract function, passing in the current values
      performUnaryOperation(X[j], Y[j], E[j], YOut[j], EOut[j]);
    }

    progress.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

/// Executes the algorithm for events
void UnaryOperation::execEvent() {
  g_log.information("Processing event workspace");

  const MatrixWorkspace_const_sptr matrixInputWS = getProperty(inputPropName());

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS = getProperty(outputPropName());
  if (matrixOutputWS != matrixInputWS) {
    matrixOutputWS = matrixInputWS->clone();
    setProperty(outputPropName(), matrixOutputWS);
  }
  auto outputWS = std::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);

  // Now fetch any properties defined by concrete algorithm
  retrieveProperties();

  auto numHistograms = static_cast<int64_t>(outputWS->getNumberHistograms());
  API::Progress prog = API::Progress(this, 0.0, 1.0, numHistograms);
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int64_t i = 0; i < numHistograms; ++i) {
    PARALLEL_START_INTERRUPT_REGION
    // switch to weighted events if needed, and use the appropriate helper
    // function
    auto &evlist = outputWS->getSpectrum(i);
    switch (evlist.getEventType()) {
    case TOF:
      // Switch to weights if needed.
      evlist.switchTo(WEIGHTED);
      /* no break */
      // Fall through

    case WEIGHTED:
      unaryOperationEventHelper(evlist.getWeightedEvents());
      break;

    case WEIGHTED_NOTIME:
      unaryOperationEventHelper(evlist.getWeightedEventsNoTime());
      break;
    }

    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  outputWS->clearMRU();
  auto inputWS = std::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  if (inputWS->getNumberEvents() != outputWS->getNumberEvents()) {
    g_log.information() << "Number of events has changed!!!\n";
  }
}

/// Helper for events, for use with different types of weighted events
template <class T> void UnaryOperation::unaryOperationEventHelper(std::vector<T> &wevector) {

  typename std::vector<T>::iterator it;
  for (it = wevector.begin(); it < wevector.end(); ++it) {
    double yout, eout;
    // Call the abstract function, passing in the current values
    performUnaryOperation(it->tof(), it->weight(), std::sqrt(it->errorSquared()), yout, eout);
    it->m_weight = static_cast<float>(yout);
    it->m_errorSquared = static_cast<float>(eout * eout);
  }
}
} // namespace Mantid::Algorithms
