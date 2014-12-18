//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnaryOperation.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Algorithms {
UnaryOperation::UnaryOperation() : API::Algorithm() {
  this->useHistogram = false;
}

UnaryOperation::~UnaryOperation() {}

/** Initialisation method.
 *  Defines input and output workspace properties
 */
void UnaryOperation::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(inputPropName(), "",
                                                         Direction::Input),
                  "The name of the input workspace");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(outputPropName(), "",
                                                         Direction::Output),
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
  EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const EventWorkspace>(in_work);
  if ((eventW != NULL) && !(this->useHistogram)) {
    this->execEvent();
    return;
  }

  MatrixWorkspace_sptr out_work = getProperty(outputPropName());
  // Only create output workspace if different to input one
  if (out_work != in_work) {
    out_work = WorkspaceFactory::Instance().create(in_work);
    if (out_work->id() == "RebinnedOutput") {
      RebinnedOutput_const_sptr intemp =
          boost::dynamic_pointer_cast<const RebinnedOutput>(in_work);
      RebinnedOutput_sptr outtemp =
          boost::dynamic_pointer_cast<RebinnedOutput>(out_work);
      for (size_t i = 0; i < outtemp->getNumberHistograms(); ++i) {
        MantidVecPtr F;
        F.access() = intemp->dataF(i);
        outtemp->setF(i, F);
      }
    }
    setProperty(outputPropName(), out_work);
  }

  // Now fetch any properties defined by concrete algorithm
  retrieveProperties();

  const size_t numSpec = in_work->getNumberHistograms();
  const size_t specSize = in_work->blocksize();
  const bool isHist = in_work->isHistogramData();

  // Initialise the progress reporting object
  Progress progress(this, 0.0, 1.0, numSpec);

  // Loop over every cell in the workspace, calling the abstract correction
  // function
  PARALLEL_FOR2(in_work, out_work)
  for (int64_t i = 0; i < int64_t(numSpec); ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Copy the X values over
    out_work->setX(i, in_work->refX(i));
    // Get references to the data
    // Output (non-const) ones first because they may copy the vector
    // if it's shared, which isn't thread-safe.
    MantidVec &YOut = out_work->dataY(i);
    MantidVec &EOut = out_work->dataE(i);
    const MantidVec &X = in_work->readX(i);
    const MantidVec &Y = in_work->readY(i);
    const MantidVec &E = in_work->readE(i);

    for (size_t j = 0; j < specSize; ++j) {
      // Use the bin centre for the X value if this is histogram data
      const double XIn = isHist ? (X[j] + X[j + 1]) / 2.0 : X[j];
      // Call the abstract function, passing in the current values
      performUnaryOperation(XIn, Y[j], E[j], YOut[j], EOut[j]);
    }

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  return;
}

/// Executes the algorithm for events
void UnaryOperation::execEvent() {
  g_log.information("Processing event workspace");

  const MatrixWorkspace_const_sptr matrixInputWS =
      this->getProperty(inputPropName());
  EventWorkspace_const_sptr inputWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

  // generate the output workspace pointer
  API::MatrixWorkspace_sptr matrixOutputWS =
      this->getProperty(outputPropName());
  EventWorkspace_sptr outputWS;
  if (matrixOutputWS == matrixInputWS) {
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
  } else {
    // Make a brand new EventWorkspace
    outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create(
            "EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    // Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS,
                                                           false);
    // You need to copy over the data as well.
    outputWS->copyDataFrom((*inputWS));

    // Cast to the matrixOutputWS and save it
    matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
    this->setProperty("OutputWorkspace", matrixOutputWS);
  }

  // Now fetch any properties defined by concrete algorithm
  retrieveProperties();

  int64_t numHistograms = static_cast<int64_t>(inputWS->getNumberHistograms());
  API::Progress prog = API::Progress(this, 0.0, 1.0, numHistograms);
  PARALLEL_FOR1(outputWS)
  for (int64_t i = 0; i < numHistograms; ++i) {
    PARALLEL_START_INTERUPT_REGION
    // switch to weighted events if needed, and use the appropriate helper
    // function
    EventList *evlist = outputWS->getEventListPtr(i);
    switch (evlist->getEventType()) {
    case TOF:
      // Switch to weights if needed.
      evlist->switchTo(WEIGHTED);
    /* no break */
    // Fall through

    case WEIGHTED:
      unaryOperationEventHelper(evlist->getWeightedEvents());
      break;

    case WEIGHTED_NOTIME:
      unaryOperationEventHelper(evlist->getWeightedEventsNoTime());
      break;
    }

    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  outputWS->clearMRU();
  if (inputWS->getNumberEvents() != outputWS->getNumberEvents()) {
    g_log.information() << "Number of events has changed!!!" << std::endl;
  }
}

/// Helper for events, for use with different types of weighted events
template <class T>
void UnaryOperation::unaryOperationEventHelper(std::vector<T> &wevector) {

  typename std::vector<T>::iterator it;
  for (it = wevector.begin(); it < wevector.end(); ++it) {
    double yout, eout;
    // Call the abstract function, passing in the current values
    performUnaryOperation(it->tof(), it->weight(),
                          std::sqrt(it->errorSquared()), yout, eout);
    it->m_weight = static_cast<float>(yout);
    it->m_errorSquared = static_cast<float>(eout * eout);
  }
}
}
}
