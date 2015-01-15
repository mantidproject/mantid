//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDEvents/FitMD.h"

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"

#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include "MantidMDEvents/MDEventFactory.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/PropertyWithValue.h"

#include <algorithm>

namespace Mantid {
namespace MDEvents {
DECLARE_DOMAINCREATOR(FitMD);

using namespace API;
using namespace Kernel;

/**
 * Default Constructor
 */
FitMD::FitMD()
    : API::IDomainCreator(NULL, std::vector<std::string>(),
                          IDomainCreator::Simple),
      m_startIndex(0), m_count(0) {}

/**
 * Constructor
 */
FitMD::FitMD(IPropertyManager *fit, const std::string &workspacePropertyName,
             IDomainCreator::DomainType domainType)
    : API::IDomainCreator(
          fit, std::vector<std::string>(1, workspacePropertyName), domainType),
      m_startIndex(0), m_count(0) {
  if (domainType != IDomainCreator::Simple) {
    throw std::runtime_error("FitMD only supports simple domains");
  }
  if (m_workspacePropertyNames.empty()) {
    throw std::runtime_error("Cannot create FitMD: no workspace given");
  }
  m_workspacePropertyName = m_workspacePropertyNames[0];
}

/**
 * Initialize from a default constructor
 *
 * @param pm :: A pointer to a property manager instance
 * @param workspacePropertyName The name of the workspace property
 * @param domainType
 */
void FitMD::initialize(Kernel::IPropertyManager *pm,
                       const std::string &workspacePropertyName,
                       IDomainCreator::DomainType domainType) {
  m_manager = pm;
  m_workspacePropertyName = workspacePropertyName;
  m_domainType = domainType;
  m_workspacePropertyNames = std::vector<std::string>(1, workspacePropertyName);
}

/**
 * Declare properties that specify the dataset within the workspace to fit to.
 * @param suffix
 * @param addProp
 */
void FitMD::declareDatasetProperties(const std::string &suffix, bool addProp) {
  if (m_domainType != Simple) {
    m_maxSizePropertyName = "MaxSize" + suffix;
    if (addProp && !m_manager->existsProperty(m_maxSizePropertyName)) {
      auto mustBePositive =
          boost::shared_ptr<BoundedValidator<int>>(new BoundedValidator<int>());
      mustBePositive->setLower(1);
      declareProperty(
          new PropertyWithValue<int>(m_maxSizePropertyName, 1, mustBePositive),
          "The maximum number of values per a simple domain.");
    }
  }
}

/**
 * Create a domain from the input workspace
 * @param domain :: The input domain
 * @param ivalues :: The calculated values
 * @param i0 :: Ignored currently
 */
void FitMD::createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                         boost::shared_ptr<API::FunctionValues> &ivalues,
                         size_t i0) {
  UNUSED_ARG(i0);
  setParameters();
  auto iterator = m_IMDWorkspace->createIterator();
  const size_t n = iterator->getDataSize();
  delete iterator;

  if (m_count == 0) {
    m_count = n;
  }

  API::FunctionDomainMD *dmd =
      new API::FunctionDomainMD(m_IMDWorkspace, m_startIndex, m_count);
  domain.reset(dmd);
  auto values = new API::FunctionValues(*domain);
  ivalues.reset(values);

  auto iter = dmd->getNextIterator();
  size_t i = 0;
  while (iter) {
    values->setFitData(i, iter->getNormalizedSignal());
    // there is a problem with errors in md workspaces. Until it is solved
    // set all weights to 1.0
    // code commented out after the next line is the normal way of setting
    // weights
    values->setFitWeight(i, 1.0);
    // double err = iter->getNormalizedError();
    // if (err <= 0.0) err = 1.0;
    // values->setFitWeight(i,1/err);
    iter = dmd->getNextIterator();
    ++i;
  };
  dmd->reset();
}

/**
 * Create an output workspace filled with data simulated with the fitting
 * function.
 * @param baseName :: The base name for the workspace
 * @param function :: The function used for the calculation
 * @param domain :: A pointer to the input domain
 * @param values :: A pointer to the calculated values
 * @param outputWorkspacePropertyName :: The property name
 */
boost::shared_ptr<API::Workspace>
FitMD::createOutputWorkspace(const std::string &baseName,
                             API::IFunction_sptr function,
                             boost::shared_ptr<API::FunctionDomain> domain,
                             boost::shared_ptr<API::FunctionValues> values,
                             const std::string &outputWorkspacePropertyName) {
  if (!values) {
    return boost::shared_ptr<API::Workspace>();
  }
  auto functionMD = boost::dynamic_pointer_cast<API::FunctionDomainMD>(domain);
  if (!functionMD) {
    return boost::shared_ptr<API::Workspace>();
  }
  API::IMDWorkspace_const_sptr domainWS = functionMD->getWorkspace();

  auto inputEventWS =
      boost::dynamic_pointer_cast<const API::IMDEventWorkspace>(domainWS);
  if (inputEventWS) {
    return createEventOutputWorkspace(baseName, *inputEventWS, *values,
                                      outputWorkspacePropertyName);
  }

  auto inputHistoWS =
      boost::dynamic_pointer_cast<const API::IMDHistoWorkspace>(domainWS);
  if (inputHistoWS) {
    return createHistoOutputWorkspace(baseName, function, inputHistoWS,
                                      outputWorkspacePropertyName);
  }

  return boost::shared_ptr<API::Workspace>();
}

/**
 * Create an output event workspace filled with data simulated with the fitting
 * function.
 * @param baseName :: The base name for the workspace
 * @param inputWorkspace :: The input workspace.
 * @param values :: The calculated values
 * @param outputWorkspacePropertyName :: The property name
 */
boost::shared_ptr<API::Workspace> FitMD::createEventOutputWorkspace(
    const std::string &baseName, const API::IMDEventWorkspace &inputWorkspace,
    const API::FunctionValues &values,
    const std::string &outputWorkspacePropertyName) {
  auto outputWS =
      MDEventFactory::CreateMDWorkspace(inputWorkspace.getNumDims(), "MDEvent");
  // Add events
  // TODO: Generalize to ND (the current framework is a bit limiting)
  auto mdWS = boost::dynamic_pointer_cast<
      MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>, 4>>(outputWS);
  if (!mdWS) {
    return boost::shared_ptr<API::Workspace>();
  }

  // Bins extents and meta data
  for (size_t i = 0; i < 4; ++i) {
    boost::shared_ptr<const Geometry::IMDDimension> inputDim =
        inputWorkspace.getDimension(i);
    Geometry::MDHistoDimensionBuilder builder;
    builder.setName(inputDim->getName());
    builder.setId(inputDim->getDimensionId());
    builder.setUnits(inputDim->getUnits());
    builder.setNumBins(inputDim->getNBins());
    builder.setMin(inputDim->getMinimum());
    builder.setMax(inputDim->getMaximum());

    outputWS->addDimension(builder.create());
  }

  // Run information
  outputWS->copyExperimentInfos(inputWorkspace);
  // Set sensible defaults for splitting behaviour
  BoxController_sptr bc = outputWS->getBoxController();
  bc->setSplitInto(3);
  bc->setSplitThreshold(3000);
  outputWS->initialize();
  outputWS->splitBox();

  auto inputIter = inputWorkspace.createIterator();
  size_t resultValueIndex(0);
  const float errorSq = 0.0;
  do {
    const size_t numEvents = inputIter->getNumEvents();
    const float signal =
        static_cast<float>(values.getCalculated(resultValueIndex));
    for (size_t i = 0; i < numEvents; ++i) {
      coord_t centers[4] = {
          inputIter->getInnerPosition(i, 0), inputIter->getInnerPosition(i, 1),
          inputIter->getInnerPosition(i, 2), inputIter->getInnerPosition(i, 3)};
      mdWS->addEvent(MDEvent<4>(signal, errorSq, inputIter->getInnerRunIndex(i),
                                inputIter->getInnerDetectorID(i), centers));
    }
    ++resultValueIndex;
  } while (inputIter->next());
  delete inputIter;

  API::MemoryManager::Instance().releaseFreeMemory();
  // This splits up all the boxes according to split thresholds and sizes.
  auto threadScheduler = new Kernel::ThreadSchedulerFIFO();
  Kernel::ThreadPool threadPool(threadScheduler);
  outputWS->splitAllIfNeeded(threadScheduler);
  threadPool.joinAll();
  outputWS->refreshCache();
  // Flush memory
  API::MemoryManager::Instance().releaseFreeMemory();

  // Store it
  if (!outputWorkspacePropertyName.empty()) {
    declareProperty(
        new API::WorkspaceProperty<API::IMDEventWorkspace>(
            outputWorkspacePropertyName, "", Direction::Output),
        "Name of the output Workspace holding resulting simulated spectrum");
    m_manager->setPropertyValue(outputWorkspacePropertyName,
                                baseName + "Workspace");
    m_manager->setProperty(outputWorkspacePropertyName, outputWS);
  }

  return outputWS;
}

/**
 * Create an output histo workspace filled with data simulated with the fitting
 * function.
 * @param baseName :: The base name for the workspace
 * @param function :: The function used for the calculation
 * @param inputWorkspace :: The input workspace
 * @param outputWorkspacePropertyName :: The property name
 */
boost::shared_ptr<API::Workspace> FitMD::createHistoOutputWorkspace(
    const std::string &baseName, API::IFunction_sptr function,
    API::IMDHistoWorkspace_const_sptr inputWorkspace,
    const std::string &outputWorkspacePropertyName) {
  // have to cast const away to be able to pass the workspace to the algorithm
  API::IMDHistoWorkspace_sptr nonConstInputWS = boost::const_pointer_cast<
      API::IMDHistoWorkspace, const API::IMDHistoWorkspace>(inputWorkspace);
  // evaluate the function on the input workspace
  auto alg = API::AlgorithmFactory::Instance().create("EvaluateMDFunction", -1);
  alg->setChild(true);
  alg->setRethrows(true);
  alg->initialize();
  alg->setProperty("Function", function);
  alg->setProperty("InputWorkspace", nonConstInputWS);
  alg->setProperty("OutputWorkspace",
                   "__FitMD_createHistoOutputWorkspace_outputWorkspace");
  alg->execute();

  // get the result
  API::IMDHistoWorkspace_sptr outputWorkspace =
      alg->getProperty("OutputWorkspace");
  // Store it
  if (!outputWorkspacePropertyName.empty()) {
    declareProperty(
        new API::WorkspaceProperty<API::IMDHistoWorkspace>(
            outputWorkspacePropertyName, "", Direction::Output),
        "Name of the output Workspace holding resulting simulated spectrum");
    m_manager->setPropertyValue(outputWorkspacePropertyName,
                                baseName + "Workspace");
    m_manager->setProperty(outputWorkspacePropertyName, outputWorkspace);
  }

  return outputWorkspace;
}

/**
 * Set all parameters
 */
void FitMD::setParameters() const {
  // if property manager is set overwrite any set parameters
  if (m_manager) {
    if (m_workspacePropertyNames.empty()) {
      throw std::runtime_error(
          "Cannot create FunctionDomainMD: no workspace given");
    }
    // get the workspace
    API::Workspace_sptr ws = m_manager->getProperty(m_workspacePropertyName);
    m_IMDWorkspace = boost::dynamic_pointer_cast<API::IMDWorkspace>(ws);
    if (!m_IMDWorkspace) {
      throw std::invalid_argument("InputWorkspace must be a MatrixWorkspace.");
    }
    if (m_domainType != Simple) {
      const int maxSizeInt = m_manager->getProperty(m_maxSizePropertyName);
      m_maxSize = static_cast<size_t>(maxSizeInt);
    }
  }
}

/**
 * Set the range
 * @param startIndex :: Starting index in the worspace
 * @param count :: Size of the domain
 */
void FitMD::setRange(size_t startIndex, size_t count) {
  m_startIndex = startIndex;
  m_count = count;
}

/**
 * @return The size of the domain to be created.
 */
size_t FitMD::getDomainSize() const {
  setParameters();
  if (!m_IMDWorkspace)
    throw std::runtime_error("FitMD: workspace wasn't defined");
  auto iterator = m_IMDWorkspace->createIterator();
  size_t n = iterator->getDataSize();
  delete iterator;
  if (m_count != 0) {
    if (m_startIndex + m_count > n)
      throw std::range_error("FitMD: index is out of range");
    n = m_count;
  }
  return n;
}

} // namespace Algorithm
} // namespace Mantid
