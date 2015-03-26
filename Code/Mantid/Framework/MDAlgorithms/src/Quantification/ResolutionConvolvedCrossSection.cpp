//
// Includes
//
#include "MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolution.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventFactory.h"

/// Parallel region start macro. Different to generic one as that is specific to
/// algorithms
/// Assumes boolean exceptionThrow has been declared
#define BEGIN_PARALLEL_REGION                                                  \
  if (!exceptionThrown && !this->cancellationRequestReceived()) {              \
    try {

/// Parallel region end macro. Different to generic one as that is specific to
/// algorithms
#define END_PARALLEL_REGION                                                    \
  }                                                                            \
  catch (std::exception & ex) {                                                \
    if (!exceptionThrown) {                                                    \
      exceptionThrown = true;                                                  \
      g_log.error() << "ResolutionConvolvedCrossSection: " << ex.what()        \
                    << "\n";                                                   \
    }                                                                          \
  }                                                                            \
  catch (...) {                                                                \
    exceptionThrown = true;                                                    \
  }                                                                            \
  }

/// Check for exceptions in parallel region
#define CHECK_PARALLEL_EXCEPTIONS                                              \
  if (exceptionThrown) {                                                       \
    g_log.debug("Exception thrown in parallel region");                        \
    throw std::runtime_error(                                                  \
        "ResolutionConvolvedCrossSection: error (see log)");                   \
  }                                                                            \
  if (this->cancellationRequestReceived()) {                                   \
    reportProgress();                                                          \
  }

namespace Mantid {
namespace MDAlgorithms {
DECLARE_FUNCTION(ResolutionConvolvedCrossSection)

namespace {
// Attribute names
const char *RESOLUTION_ATTR = "ResolutionFunction";
const char *FOREGROUND_ATTR = "ForegroundModel";
const char *SIMULATION_ATTR = "Simulation";

/// static logger
Kernel::Logger g_log("ResolutionConvolvedCrossSection");
}

/**
 * Constructor
 */
ResolutionConvolvedCrossSection::ResolutionConvolvedCrossSection()
    : ParamFunction(), IFunctionMD(), m_simulation(false), m_convolution(NULL),
      m_inputWS() {}

/**
 * Destructor
 */
ResolutionConvolvedCrossSection::~ResolutionConvolvedCrossSection() {
  delete m_convolution;
}

/**
 * Declare the attributes associated with this function
 */
void ResolutionConvolvedCrossSection::declareAttributes() {
  declareAttribute(RESOLUTION_ATTR, IFunction::Attribute(""));
  declareAttribute(FOREGROUND_ATTR, IFunction::Attribute(""));

  declareAttribute(SIMULATION_ATTR, IFunction::Attribute(m_simulation));
}

/**
 * Declares any model parameters.
 */
void ResolutionConvolvedCrossSection::declareParameters() {}

void
ResolutionConvolvedCrossSection::function(const API::FunctionDomain &domain,
                                          API::FunctionValues &values) const {
  const API::FunctionDomainMD *domainMD =
      dynamic_cast<const API::FunctionDomainMD *>(&domain);
  if (!domainMD) {
    throw std::invalid_argument(
        "Expected FunctionDomainMD in ResolutionConvolvedCrossSection");
  }

  std::vector<API::IMDIterator *> iterators = m_inputWS->createIterators(
      API::FrameworkManager::Instance().getNumOMPThreads());
  const int nthreads = static_cast<int>(iterators.size());
  std::vector<size_t> resultOffsets(nthreads, 0);

  // Each calculated result needs to be at the correct index in the according to
  // its
  // position. The order here is the same that would be if it were computed
  // serially.
  // The offsets vector sets the size of the offset into the calculated results
  // for each
  // thread
  if (nthreads > 1) {
    // The first result will have offset of 0 so that is already correct
    size_t totalBoxes = iterators[0]->getDataSize();
    for (int i = 1; i < nthreads; ++i) {
      // The offset is simply the total number of boxes before it
      resultOffsets[i] = totalBoxes;
      totalBoxes += iterators[i]->getDataSize();
    }
  }

  bool exceptionThrown = false; // required for *_PARALLEL_* macros
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < nthreads; ++i) {
    API::IMDIterator *boxIterator = iterators[i];
    const size_t resultsOffset = resultOffsets[i];

    size_t boxIndex(0);
    do {
      BEGIN_PARALLEL_REGION // Not standard macros. See top of file for reason

          storeCalculatedWithMutex(resultsOffset + boxIndex,
                                   functionMD(*boxIterator), values);
      ++boxIndex;

      END_PARALLEL_REGION // Not standard macros. See top of file for reason
    } while (boxIterator->next());
  }
  CHECK_PARALLEL_EXCEPTIONS // Not standard macros. See top of file for reason

      for (auto it = iterators.begin(); it != iterators.end(); ++it) {
    API::IMDIterator *boxIterator = *it;
    delete boxIterator;
  }
}

/**
 * Returns the value of the function for the given MD box. For each MDPoint
 * within the box the convolution of the current model with the resolution
 * is computed at summed. The average of this over the number of events
 * is returned
 * @param box An iterator pointing at the MDBox being looked at
 * @return The current value of the function
 */
double ResolutionConvolvedCrossSection::functionMD(
    const Mantid::API::IMDIterator &box) const {
  assert(m_convolution);
  const size_t numEvents = box.getNumEvents();
  if (numEvents == 0)
    return 0.0;

  double totalBoxSignal(0.0);
  for (size_t j = 0; j < numEvents; ++j) {
    const uint16_t innerRun = box.getInnerRunIndex(j);
    const double contribution = m_convolution->signal(box, innerRun, j);
    if (m_simulation) {
      coord_t centers[4] = {
          box.getInnerPosition(j, 0), box.getInnerPosition(j, 1),
          box.getInnerPosition(j, 2), box.getInnerPosition(j, 3)};
      PARALLEL_CRITICAL(ResolutionConvolvedCrossSection_functionMD) {
        m_simulatedEvents.insert(
            m_simulatedEvents.end(),
            MDEvents::MDEvent<4>(static_cast<float>(contribution), 0.0f,
                                 innerRun, box.getInnerDetectorID(j), centers));
      }
    }

    totalBoxSignal += contribution;
    reportProgress();
  }

  // Return the mean
  return totalBoxSignal / static_cast<double>(numEvents);
}

/**
 * Store the simulated events in the given workspace. This clears the calculated
 * values
 * @param resultWS :: An output workspace that has all of its meta-data set up
 * and just needs to
 * be filled with events.
 */
void ResolutionConvolvedCrossSection::storeSimulatedEvents(
    const API::IMDEventWorkspace_sptr &resultWS) {
  auto outputWS =
      boost::dynamic_pointer_cast<MDEvents::MDEventWorkspace4>(resultWS);
  if (!outputWS) {
    throw std::invalid_argument(
        "ResolutionConvolvedCrossSection currently only supports 4 dimensions");
  }

  auto iterEnd = m_simulatedEvents.end();
  for (auto iter = m_simulatedEvents.begin(); iter != iterEnd; ++iter) {
    outputWS->addEvent(*iter);
  }
  m_simulatedEvents.clear();

  API::MemoryManager::Instance().releaseFreeMemory();
  // This splits up all the boxes according to split thresholds and sizes.
  auto threadScheduler = new Kernel::ThreadSchedulerFIFO();
  Kernel::ThreadPool threadPool(threadScheduler);
  outputWS->splitAllIfNeeded(threadScheduler);
  threadPool.joinAll();
  outputWS->refreshCache();

  // Flush memory
  API::MemoryManager::Instance().releaseFreeMemory();
}

/**
 * Override the call to set the workspace here to store it so we can access it
 * throughout the evaluation
 * @param workspace :: A workspace containing the instrument definition
 */
void ResolutionConvolvedCrossSection::setWorkspace(
    boost::shared_ptr<const API::Workspace> workspace) {
  if (!m_convolution)
    return;

  m_inputWS =
      boost::dynamic_pointer_cast<const API::IMDEventWorkspace>(workspace);
  if (!m_inputWS) {
    throw std::invalid_argument("ResolutionConvolvedCrossSection can only be "
                                "used with MD event workspaces");
  }
  IFunctionMD::setWorkspace(workspace);
  m_convolution->preprocess(m_inputWS);
}

/**
 * Do any final setup before fitting starts
 */
void ResolutionConvolvedCrossSection::setUpForFit() {
  // Consistency check
  const std::string fgModelName = getAttribute(FOREGROUND_ATTR).asString();
  if (fgModelName.empty()) {
    throw std::invalid_argument(
        "ResolutionConvolvedCrossSection - No foreground model has been set.");
  }
  const std::string convolutionType = getAttribute(RESOLUTION_ATTR).asString();
  if (convolutionType.empty()) {
    throw std::invalid_argument(
        "ResolutionConvolvedCrossSection - No convolution type has been set.");
  }
  m_convolution->setUpForFit();
}

/**
 * Returns an estimate of the number of progress reports a single evaluation of
 * the function will have.
 * @return
 */
int64_t ResolutionConvolvedCrossSection::estimateNoProgressCalls() const {
  int64_t ncalls(1);
  if (m_inputWS) {
    ncalls = static_cast<int64_t>(m_inputWS->getNPoints());
  }
  return ncalls;
}

/**
 *  Set a value to a named attribute
 *  @param name :: The name of the attribute
 *  @param value :: The value of the attribute
 */
void ResolutionConvolvedCrossSection::setAttribute(
    const std::string &name, const API::IFunction::Attribute &value) {
  storeAttributeValue(name, value);
  const std::string fgModelName = getAttribute(FOREGROUND_ATTR).asString();
  const std::string convolutionType = getAttribute(RESOLUTION_ATTR).asString();

  if (!convolutionType.empty() && !fgModelName.empty()) {
    setupResolutionFunction(convolutionType, fgModelName);
  }
  if (name == SIMULATION_ATTR)
    m_simulation = value.asBool();
  else if (name != FOREGROUND_ATTR && name != RESOLUTION_ATTR) {
    m_convolution->setAttribute(name, value);
  }
}

/**
 * Set a pointer to the concrete convolution object from a named implementation.
 * @param name :: The name of a convolution type
 * @param fgModelName :: The foreground model name selected
 */
void ResolutionConvolvedCrossSection::setupResolutionFunction(
    const std::string &name, const std::string &fgModelName) {
  if (m_convolution)
    return; // Done

  m_convolution = MDResolutionConvolutionFactory::Instance().createConvolution(
      name, fgModelName, *this);
  // Pass on the attributes
  auto names = m_convolution->getAttributeNames();
  for (auto iter = names.begin(); iter != names.end(); ++iter) {
    this->declareAttribute(*iter, m_convolution->getAttribute(*iter));
  }
  // Pull the foreground parameters on to here
  const ForegroundModel &fgModel = m_convolution->foregroundModel();
  const size_t nparams = fgModel.nParams();
  for (size_t i = 0; i < nparams; ++i) {
    this->declareParameter(fgModel.parameterName(i),
                           fgModel.getInitialParameterValue(i),
                           fgModel.parameterDescription(i));
  }
  // Pull the foreground attributes on to here
  names = fgModel.getAttributeNames();
  for (auto iter = names.begin(); iter != names.end(); ++iter) {
    this->declareAttribute(*iter, fgModel.getAttribute(*iter));
  }
}

/**
 * @param index Index value into functionValues array
 * @param signal Calculated signal value
 * @param functionValues [InOut] Final calculated values
 */
void ResolutionConvolvedCrossSection::storeCalculatedWithMutex(
    const size_t index, const double signal,
    API::FunctionValues &functionValues) const {
  Poco::FastMutex::ScopedLock lock(m_valuesMutex);
  functionValues.setCalculated(index, signal);
}
}
}
