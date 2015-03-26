#include "MantidMDAlgorithms/Quantification/SimulateResolutionConvolvedModel.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModelFactory.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolutionFactory.h"
#include "MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h"

namespace Mantid {
namespace MDAlgorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SimulateResolutionConvolvedModel)

using namespace API;
using namespace Kernel;
using Geometry::MDHistoDimensionBuilder;
using Geometry::Vec_MDHistoDimensionBuilder;
using MDEvents::MDEventWorkspace;
using MDEvents::MDEvent;

namespace {
// Property names
const char *INPUT_WS_NAME = "InputWorkspace";
const char *SIMULATED_NAME = "OutputWorkspace";
const char *RESOLUTION_NAME = "ResolutionFunction";
const char *FOREGROUND_NAME = "ForegroundModel";
const char *PARS_NAME = "Parameters";
const char *APPEND_NAME = "AppendToExisting";
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SimulateResolutionConvolvedModel::name() const {
  return "SimulateResolutionConvolvedModel";
}

/// Algorithm's version for identification. @see Algorithm::version
int SimulateResolutionConvolvedModel::version() const { return 1; }

/**
 * Returns the number of iterations that should be performed
 * @returns 1 for the simulation
 */
int SimulateResolutionConvolvedModel::niterations() const { return 1; }

/**
 * Initialize the algorithm's properties.
 */
void SimulateResolutionConvolvedModel::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(INPUT_WS_NAME, "",
                                                           Direction::Input),
                  "The input MDEvent workspace");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(SIMULATED_NAME, "",
                                                           Direction::Output),
                  "The simulated output workspace");

  std::vector<std::string> models =
      MDResolutionConvolutionFactory::Instance().getKeys();
  declareProperty(RESOLUTION_NAME, "",
                  boost::make_shared<ListValidator<std::string>>(models),
                  "The name of a resolution model", Direction::Input);

  models = ForegroundModelFactory::Instance().getKeys();
  declareProperty(FOREGROUND_NAME, "",
                  boost::make_shared<ListValidator<std::string>>(models),
                  "The name of a foreground function", Direction::Input);

  declareProperty(PARS_NAME, "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The parameters/attributes for the function & model. See Fit "
                  "documentation for format",
                  Direction::Input);

  declareProperty(APPEND_NAME, false, "If true then the simulated events will "
                                      "be added to an existing workspace. If "
                                      "the workspace does "
                                      "not exist then it is created",
                  Direction::Input);
}

/**
 * Executes the simulation
 */
void SimulateResolutionConvolvedModel::exec() {
  m_inputWS = getProperty("InputWorkspace");
  // First estimate of progress calls
  API::Progress progress(this, 0.0, 1.0,
                         static_cast<size_t>(m_inputWS->getNPoints()));
  progress.report("Caching simulation input");
  auto resolution = createFunction();
  createDomains();

  // Do the real work
  progress.setNumSteps(resolution->estimateNoProgressCalls());
  resolution->setProgressReporter(&progress);
  resolution->function(*m_domain, *m_calculatedValues);

  // If output workspace exists just add the events to that
  IMDEventWorkspace_sptr existingWS = getProperty(SIMULATED_NAME);
  bool append = getProperty(APPEND_NAME);
  if (append && existingWS) {
    m_outputWS = boost::dynamic_pointer_cast<QOmegaWorkspace>(existingWS);
  } else {
    createOutputWorkspace();
  }
  auto functionMD =
      boost::dynamic_pointer_cast<ResolutionConvolvedCrossSection>(resolution);
  functionMD->storeSimulatedEvents(m_outputWS);

  this->setProperty<IMDEventWorkspace_sptr>(SIMULATED_NAME, m_outputWS);
}

/**
 * @return The new MD function to be evaluated
 *
 */
boost::shared_ptr<API::IFunction>
SimulateResolutionConvolvedModel::createFunction() const {
  const std::string functionStr = this->createFunctionString();
  auto ifunction = FunctionFactory::Instance().createInitialized(functionStr);
  auto functionMD =
      boost::dynamic_pointer_cast<ResolutionConvolvedCrossSection>(ifunction);
  if (!functionMD) {
    throw std::invalid_argument("Function created is not the expected "
                                "ResolutionConvolvedCrossSection function. "
                                "Contact support.");
  }
  ifunction->setAttribute("Simulation", IFunction::Attribute(true));
  ifunction->setWorkspace(m_inputWS);
  ifunction->setUpForFit();
  return ifunction;
}

/**
 * Create the input & output domains from the input workspace
 */
void SimulateResolutionConvolvedModel::createDomains() {
  m_domain.reset(new FunctionDomainMD(m_inputWS, 0, 0));
  m_calculatedValues.reset(new FunctionValues(*m_domain));
}

/**
 *  Generate the output MD workspace that is a result of the simulation
 * @return The generated MD event workspace
 */
void SimulateResolutionConvolvedModel::createOutputWorkspace() {
  m_outputWS = boost::shared_ptr<QOmegaWorkspace>(new QOmegaWorkspace);

  // Bins extents and meta data
  // Set sensible defaults for splitting behaviour
  BoxController_sptr bc = m_outputWS->getBoxController();
  bc->setSplitThreshold(3000);
  for (size_t i = 0; i < 4; ++i) {
    boost::shared_ptr<const Geometry::IMDDimension> inputDim =
        m_inputWS->getDimension(i);
    MDHistoDimensionBuilder builder;
    builder.setName(inputDim->getName());
    builder.setId(inputDim->getDimensionId());
    builder.setUnits(inputDim->getUnits());
    builder.setNumBins(inputDim->getNBins());
    bc->setSplitInto(i, inputDim->getNBins());
    builder.setMin(inputDim->getMinimum());
    builder.setMax(inputDim->getMaximum());

    m_outputWS->addDimension(builder.create());
  }
  // Run information
  m_outputWS->copyExperimentInfos(*m_inputWS);
  // Coordinates
  m_outputWS->setCoordinateSystem(m_inputWS->getSpecialCoordinateSystem());

  m_outputWS->initialize();
  m_outputWS->splitBox(); // Make grid box
}

} // namespace MDAlgorithms
} // namespace Mantid
