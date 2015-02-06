#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/PropertyManager.h"
#include <stdexcept>
#include "Poco/Path.h"
#ifdef MPI_BUILD
#include <boost/mpi.hpp>
#endif

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
DataProcessorAlgorithm::DataProcessorAlgorithm() : API::Algorithm() {
  m_loadAlg = "Load";
  m_accumulateAlg = "Plus";
  m_loadAlgFileProp = "Filename";
  m_useMPI = false;
  enableHistoryRecordingForChild(true);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
DataProcessorAlgorithm::~DataProcessorAlgorithm() {}

//---------------------------------------------------------------------------------------------
/** Create a Child Algorithm.  A call to this method creates a child algorithm
*object.
*  Using this mechanism instead of creating daughter
*  algorithms directly via the new operator is prefered since then
*  the framework can take care of all of the necessary book-keeping.
*
*  Overrides the method of the same name in Algorithm to enable history tracking
*by default.
*
*  @param name ::           The concrete algorithm class of the Child Algorithm
*  @param startProgress ::  The percentage progress value of the overall
*algorithm where this child algorithm starts
*  @param endProgress ::    The percentage progress value of the overall
*algorithm where this child algorithm ends
*  @param enableLogging ::  Set to false to disable logging from the child
*algorithm
*  @param version ::        The version of the child algorithm to create. By
*default gives the latest version.
*  @return shared pointer to the newly created algorithm object
*/
boost::shared_ptr<Algorithm> DataProcessorAlgorithm::createChildAlgorithm(
    const std::string &name, const double startProgress,
    const double endProgress, const bool enableLogging, const int &version) {
  // call parent method to create the child algorithm
  auto alg = Algorithm::createChildAlgorithm(name, startProgress, endProgress,
                                             enableLogging, version);
  alg->enableHistoryRecordingForChild(this->isRecordingHistoryForChild());
  if (this->isRecordingHistoryForChild()) {
    // pass pointer to the history object created in Algorithm to the child
    alg->trackAlgorithmHistory(m_history);
  }
  return alg;
}

void DataProcessorAlgorithm::setLoadAlg(const std::string &alg) {
  if (alg.empty())
    throw std::invalid_argument("Cannot set load algorithm to empty string");
  m_loadAlg = alg;
}

void
DataProcessorAlgorithm::setLoadAlgFileProp(const std::string &filePropName) {
  if (filePropName.empty()) {
    throw std::invalid_argument(
        "Cannot set the load algorithm file property name");
  }
  m_loadAlgFileProp = filePropName;
}

void DataProcessorAlgorithm::setAccumAlg(const std::string &alg) {
  if (alg.empty())
    throw std::invalid_argument(
        "Cannot set accumulate algorithm to empty string");
  m_accumulateAlg = alg;
}

ITableWorkspace_sptr DataProcessorAlgorithm::determineChunk() {
  throw std::runtime_error(
      "DataProcessorAlgorithm::determineChunk is not implemented");
}

void DataProcessorAlgorithm::loadChunk() {

  throw std::runtime_error(
      "DataProcessorAlgorithm::loadChunk is not implemented");
}

/**
 * Assemble the partial workspaces from all MPI processes
 * @param partialWSName :: Name of the workspace to assemble
 * @param outputWSName :: Name of the assembled workspace (available in main
 * thread only)
 */
Workspace_sptr
DataProcessorAlgorithm::assemble(const std::string &partialWSName,
                                 const std::string &outputWSName) {
  std::string threadOutput = partialWSName;
#ifdef MPI_BUILD
  Workspace_sptr partialWS =
      AnalysisDataService::Instance().retrieve(partialWSName);
  IAlgorithm_sptr gatherAlg = createChildAlgorithm("GatherWorkspaces");
  gatherAlg->setLogging(true);
  gatherAlg->setAlwaysStoreInADS(true);
  gatherAlg->setProperty("InputWorkspace", partialWS);
  gatherAlg->setProperty("PreserveEvents", true);
  gatherAlg->setPropertyValue("OutputWorkspace", outputWSName);
  gatherAlg->execute();

  if (isMainThread())
    threadOutput = outputWSName;
#else
  UNUSED_ARG(outputWSName)
#endif
  Workspace_sptr outputWS =
      AnalysisDataService::Instance().retrieve(threadOutput);
  return outputWS;
}

/**
 * Save a workspace as a nexus file, with check for which thread
 * we are executing in.
 * @param outputWSName :: Name of the workspace to save
 * @param outputFile :: Path to the Nexus file to save
 */
void DataProcessorAlgorithm::saveNexus(const std::string &outputWSName,
                                       const std::string &outputFile) {
  bool saveOutput = true;
#ifdef MPI_BUILD
  if (boost::mpi::communicator().rank() > 0)
    saveOutput = false;
#endif

  if (saveOutput && outputFile.size() > 0) {
    IAlgorithm_sptr saveAlg = createChildAlgorithm("SaveNexus");
    saveAlg->setPropertyValue("Filename", outputFile);
    saveAlg->setPropertyValue("InputWorkspace", outputWSName);
    saveAlg->execute();
  }
}

/// Return true if we are running on the main thread
bool DataProcessorAlgorithm::isMainThread() {
  bool mainThread;
#ifdef MPI_BUILD
  mainThread = (boost::mpi::communicator().rank() == 0);
#else
  mainThread = true;
#endif
  return mainThread;
}

/// Return the number of MPI processes running
int DataProcessorAlgorithm::getNThreads() {
#ifdef MPI_BUILD
  return boost::mpi::communicator().size();
#else
  return 1;
#endif
}

/**
 * Determine what kind of input data we have and load it
 * @param inputData :: File path or workspace name
 * @param loadQuiet :: If true then the output is not stored in the ADS
 */
Workspace_sptr DataProcessorAlgorithm::load(const std::string &inputData,
                                            const bool loadQuiet) {
  Workspace_sptr inputWS;

  // First, check whether we have the name of an existing workspace
  if (AnalysisDataService::Instance().doesExist(inputData)) {
    inputWS = AnalysisDataService::Instance().retrieve(inputData);
  } else {
    std::string foundFile = FileFinder::Instance().getFullPath(inputData);
    if (foundFile.empty()) {
      // Get facility extensions
      FacilityInfo facilityInfo = ConfigService::Instance().getFacility();
      const std::vector<std::string> facilityExts = facilityInfo.extensions();
      foundFile = FileFinder::Instance().findRun(inputData, facilityExts);
    }

    if (!foundFile.empty()) {
      Poco::Path p(foundFile);
      const std::string outputWSName = p.getBaseName();

      IAlgorithm_sptr loadAlg = createChildAlgorithm(m_loadAlg);
      loadAlg->setProperty(m_loadAlgFileProp, foundFile);
      if (!loadQuiet) {
        loadAlg->setAlwaysStoreInADS(true);
      }

// Set up MPI if available
#ifdef MPI_BUILD
      // First, check whether the loader allows use to chunk the data
      if (loadAlg->existsProperty("ChunkNumber") &&
          loadAlg->existsProperty("TotalChunks")) {
        m_useMPI = true;
        // The communicator containing all processes
        boost::mpi::communicator world;
        g_log.notice() << "Chunk/Total: " << world.rank() + 1 << "/"
                       << world.size() << std::endl;
        loadAlg->setPropertyValue("OutputWorkspace", outputWSName);
        loadAlg->setProperty("ChunkNumber", world.rank() + 1);
        loadAlg->setProperty("TotalChunks", world.size());
      }
#endif
      loadAlg->execute();

      if (loadQuiet) {
        inputWS = loadAlg->getProperty("OutputWorkspace");
      } else {
        inputWS = AnalysisDataService::Instance().retrieve(outputWSName);
      }
    } else
      throw std::runtime_error(
          "DataProcessorAlgorithm::load could process any data");
  }
  return inputWS;
}

/**
 * Get the property manager object of a given name from the property manager
 * data service, or create a new one.
 * @param propertyManager :: Name of the property manager to retrieve
 */
boost::shared_ptr<PropertyManager> DataProcessorAlgorithm::getProcessProperties(
    const std::string &propertyManager) {
  boost::shared_ptr<PropertyManager> processProperties;
  if (PropertyManagerDataService::Instance().doesExist(propertyManager)) {
    processProperties =
        PropertyManagerDataService::Instance().retrieve(propertyManager);
  } else {
    getLogger().notice() << "Could not find property manager" << std::endl;
    processProperties = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(propertyManager,
                                                        processProperties);
  }
  return processProperties;
}

std::vector<std::string>
DataProcessorAlgorithm::splitInput(const std::string &input) {
  UNUSED_ARG(input);
  throw std::runtime_error(
      "DataProcessorAlgorithm::splitInput is not implemented");
}

void DataProcessorAlgorithm::forwardProperties() {
  throw std::runtime_error(
      "DataProcessorAlgorithm::forwardProperties is not implemented");
}

//------------------------------------------------------------------------------------------
// Binary opration implementations for DPA so it can record history
//------------------------------------------------------------------------------------------

/**
 * Divide a matrix workspace by another matrix workspace
 * @param lhs :: the workspace on the left hand side of the divide symbol
 * @param rhs :: the workspace on the right hand side of the divide symbol
 * @return matrix workspace resulting from the operation
 */
MatrixWorkspace_sptr
DataProcessorAlgorithm::divide(const MatrixWorkspace_sptr lhs,
                               const MatrixWorkspace_sptr rhs) {
  return this->executeBinaryAlgorithm<
      MatrixWorkspace_sptr, MatrixWorkspace_sptr, MatrixWorkspace_sptr>(
      "Divide", lhs, rhs);
}

/**
 * Divide a matrix workspace by a single value
 * @param lhs :: the workspace on the left hand side of the divide symbol
 * @param rhsValue :: the value on the right hand side of the divide symbol
 * @return matrix workspace resulting from the operation
 */
MatrixWorkspace_sptr
DataProcessorAlgorithm::divide(const MatrixWorkspace_sptr lhs,
                               const double &rhsValue) {
  return this->executeBinaryAlgorithm<
      MatrixWorkspace_sptr, MatrixWorkspace_sptr, MatrixWorkspace_sptr>(
      "Divide", lhs, createWorkspaceSingleValue(rhsValue));
}

/**
 * Multiply a matrix workspace by another matrix workspace
 * @param lhs :: the workspace on the left hand side of the multiplication
 * symbol
 * @param rhs :: the workspace on the right hand side of the multiplication
 * symbol
 * @return matrix workspace resulting from the operation
 */
MatrixWorkspace_sptr
DataProcessorAlgorithm::multiply(const MatrixWorkspace_sptr lhs,
                                 const MatrixWorkspace_sptr rhs) {
  return this->executeBinaryAlgorithm<
      MatrixWorkspace_sptr, MatrixWorkspace_sptr, MatrixWorkspace_sptr>(
      "Divide", lhs, rhs);
}

/**
 * Multiply a matrix workspace by a single value
 * @param lhs :: the workspace on the left hand side of the multiplication
 * symbol
 * @param rhsValue :: the value on the right hand side of the multiplication
 * symbol
 * @return matrix workspace resulting from the operation
 */
MatrixWorkspace_sptr
DataProcessorAlgorithm::multiply(const MatrixWorkspace_sptr lhs,
                                 const double &rhsValue) {
  return this->executeBinaryAlgorithm<
      MatrixWorkspace_sptr, MatrixWorkspace_sptr, MatrixWorkspace_sptr>(
      "Multiply", lhs, createWorkspaceSingleValue(rhsValue));
}

/**
 * Add a matrix workspace to another matrix workspace
 * @param lhs :: the workspace on the left hand side of the addition symbol
 * @param rhs :: the workspace on the right hand side of the addition symbol
 * @return matrix workspace resulting from the operation
 */
MatrixWorkspace_sptr
DataProcessorAlgorithm::plus(const MatrixWorkspace_sptr lhs,
                             const MatrixWorkspace_sptr rhs) {
  return this->executeBinaryAlgorithm<
      MatrixWorkspace_sptr, MatrixWorkspace_sptr, MatrixWorkspace_sptr>(
      "Plus", lhs, rhs);
}

/**
 * Add a single value to another matrix workspace
 * @param lhs :: the workspace on the left hand side of the addition symbol
 * @param rhsValue :: the value on the right hand side of the addition symbol
 * @return matrix workspace resulting from the operation
 */
MatrixWorkspace_sptr
DataProcessorAlgorithm::plus(const MatrixWorkspace_sptr lhs,
                             const double &rhsValue) {
  return this->executeBinaryAlgorithm<
      MatrixWorkspace_sptr, MatrixWorkspace_sptr, MatrixWorkspace_sptr>(
      "Plus", lhs, createWorkspaceSingleValue(rhsValue));
}

/**
 * Subract a matrix workspace from another matrix workspace
 * @param lhs :: the workspace on the left hand side of the subtraction symbol
 * @param rhs :: the workspace on the right hand side of the subtraction symbol
 * @return matrix workspace resulting from the operation
 */
MatrixWorkspace_sptr
DataProcessorAlgorithm::minus(const MatrixWorkspace_sptr lhs,
                              const MatrixWorkspace_sptr rhs) {
  return this->executeBinaryAlgorithm<
      MatrixWorkspace_sptr, MatrixWorkspace_sptr, MatrixWorkspace_sptr>(
      "Minus", lhs, rhs);
}

/**
 * Subract a single value from a matrix workspace
 * @param lhs :: the workspace on the left hand side of the subtraction symbol
 * @param rhsValue :: the workspace on the right hand side of the subtraction
 * symbol
 * @return matrix workspace resulting from the operation
 */
MatrixWorkspace_sptr
DataProcessorAlgorithm::minus(const MatrixWorkspace_sptr lhs,
                              const double &rhsValue) {
  return this->executeBinaryAlgorithm<
      MatrixWorkspace_sptr, MatrixWorkspace_sptr, MatrixWorkspace_sptr>(
      "Minus", lhs, createWorkspaceSingleValue(rhsValue));
}

/**
 * Create a workspace that contains just a single Y value.
 * @param rhsValue :: the value to convert to a single value matrix workspace
 * @return matrix workspace resulting from the operation
 */
MatrixWorkspace_sptr
DataProcessorAlgorithm::createWorkspaceSingleValue(const double &rhsValue) {
  MatrixWorkspace_sptr retVal =
      WorkspaceFactory::Instance().create("WorkspaceSingleValue", 1, 1, 1);
  retVal->dataY(0)[0] = rhsValue;

  return retVal;
}

} // namespace Mantid
} // namespace API
