// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/System.h"
#include "Poco/Path.h"
#include <stdexcept>
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
template <class Base>
GenericDataProcessorAlgorithm<Base>::GenericDataProcessorAlgorithm()
    : m_useMPI(false), m_loadAlg("Load"), m_accumulateAlg("Plus"),
      m_loadAlgFileProp("Filename"),
      m_propertyManagerPropertyName("ReductionProperties") {
  Base::enableHistoryRecordingForChild(true);
}

//---------------------------------------------------------------------------------------------
/** Create a Child Algorithm.  A call to this method creates a child algorithm
 *object.
 *  Using this mechanism instead of creating daughter
 *  algorithms directly via the new operator is prefered since then
 *  the framework can take care of all of the necessary book-keeping.
 *
 *  Overrides the method of the same name in Algorithm to enable history
 *tracking by default.
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
template <class Base>
boost::shared_ptr<Algorithm>
GenericDataProcessorAlgorithm<Base>::createChildAlgorithm(
    const std::string &name, const double startProgress,
    const double endProgress, const bool enableLogging, const int &version) {
  // call parent method to create the child algorithm
  auto alg = Algorithm::createChildAlgorithm(name, startProgress, endProgress,
                                             enableLogging, version);
  alg->enableHistoryRecordingForChild(this->isRecordingHistoryForChild());
  if (this->isRecordingHistoryForChild()) {
    // pass pointer to the history object created in Algorithm to the child
    alg->trackAlgorithmHistory(Base::m_history);
  }
  return alg;
}

template <class Base>
void GenericDataProcessorAlgorithm<Base>::setLoadAlg(const std::string &alg) {
  if (alg.empty())
    throw std::invalid_argument("Cannot set load algorithm to empty string");
  m_loadAlg = alg;
}

template <class Base>
void GenericDataProcessorAlgorithm<Base>::setLoadAlgFileProp(
    const std::string &filePropName) {
  if (filePropName.empty()) {
    throw std::invalid_argument(
        "Cannot set the load algorithm file property name");
  }
  m_loadAlgFileProp = filePropName;
}

template <class Base>
void GenericDataProcessorAlgorithm<Base>::setAccumAlg(const std::string &alg) {
  if (alg.empty())
    throw std::invalid_argument(
        "Cannot set accumulate algorithm to empty string");
  m_accumulateAlg = alg;
}

template <class Base>
void GenericDataProcessorAlgorithm<Base>::setPropManagerPropName(
    const std::string &propName) {
  m_propertyManagerPropertyName = propName;
}

/**
 * Declare mapping of property name to name in the PropertyManager. This is used
 *by
 * getPropertyValue(const string &) and getProperty(const string&).
 *
 * @param nameInProp Name of the property as declared in Algorithm::init().
 * @param nameInPropManager Name of the property in the PropertyManager.
 */
template <class Base>
void GenericDataProcessorAlgorithm<Base>::mapPropertyName(
    const std::string &nameInProp, const std::string &nameInPropManager) {
  m_nameToPMName[nameInProp] = nameInPropManager;
}

/**
 * Copy a property from an existing algorithm.
 *
 * @warning This only works if your algorithm is in the WorkflowAlgorithms
 *sub-project.
 *
 * @param alg
 * @param name
 *
 * @throws std::runtime_error If you ask to copy a non-existent property
 */
template <class Base>
void GenericDataProcessorAlgorithm<Base>::copyProperty(
    API::Algorithm_sptr alg, const std::string &name) {
  if (!alg->existsProperty(name)) {
    std::stringstream msg;
    msg << "Algorithm \"" << alg->name() << "\" does not have property \""
        << name << "\"";
    throw std::runtime_error(msg.str());
  }

  auto prop = alg->getPointerToProperty(name);
  Base::declareProperty(std::unique_ptr<Property>(prop->clone()),
                        prop->documentation());
}

/**
 * Get the property held by this object. If the value is the default see if it
 * is contained in the PropertyManager. @see Algorithm::getPropertyValue(const
 *string &)
 *
 * @param name
 * @return
 */
template <class Base>
std::string GenericDataProcessorAlgorithm<Base>::getPropertyValue(
    const std::string &name) const {
  // explicitly specifying a property wins
  if (!Base::isDefault(name)) {
    return Algorithm::getPropertyValue(name);
  }

  // return it if it is in the held property manager
  auto mapping = m_nameToPMName.find(name);
  if (mapping != m_nameToPMName.end()) {
    auto pm = this->getProcessProperties();
    if (pm->existsProperty(mapping->second)) {
      return pm->getPropertyValue(mapping->second);
    }
  }

  // let the parent class version win
  return Algorithm::getPropertyValue(name);
}

/**
 * Get the property held by this object. If the value is the default see if it
 * contained in the PropertyManager. @see Algorithm::getProperty(const string&)
 *
 * @param name
 * @return
 */
template <class Base>
PropertyManagerOwner::TypedValue
GenericDataProcessorAlgorithm<Base>::getProperty(
    const std::string &name) const {
  // explicitely specifying a property wins
  if (!Base::isDefault(name)) {
    return Base::getProperty(name);
  }

  // return it if it is in the held property manager
  auto mapping = m_nameToPMName.find(name);
  if (mapping != m_nameToPMName.end()) {
    auto pm = this->getProcessProperties();
    if (pm->existsProperty(mapping->second)) {
      return pm->getProperty(mapping->second);
    }
  }

  // let the parent class version win
  return Algorithm::getProperty(name);
}

template <class Base>
ITableWorkspace_sptr GenericDataProcessorAlgorithm<Base>::determineChunk(
    const std::string &filename) {
  UNUSED_ARG(filename);

  throw std::runtime_error(
      "DataProcessorAlgorithm::determineChunk is not implemented");
}

template <class Base>
MatrixWorkspace_sptr
GenericDataProcessorAlgorithm<Base>::loadChunk(const size_t rowIndex) {
  UNUSED_ARG(rowIndex);

  throw std::runtime_error(
      "DataProcessorAlgorithm::loadChunk is not implemented");
}

/**
 * Assemble the partial workspaces from all MPI processes
 * @param partialWS :: workspace to assemble
 * thread only)
 */
template <class Base>
Workspace_sptr
GenericDataProcessorAlgorithm<Base>::assemble(Workspace_sptr partialWS) {
  Workspace_sptr outputWS = partialWS;
#ifdef MPI_BUILD
  IAlgorithm_sptr gatherAlg = createChildAlgorithm("GatherWorkspaces");
  gatherAlg->setLogging(true);
  gatherAlg->setAlwaysStoreInADS(true);
  gatherAlg->setProperty("InputWorkspace", partialWS);
  gatherAlg->setProperty("PreserveEvents", true);
  gatherAlg->setPropertyValue("OutputWorkspace", "_total");
  gatherAlg->execute();

  if (isMainThread()) {
    outputWS = AnalysisDataService::Instance().retrieve("_total");
  }
#endif

  return outputWS;
}

/**
 * Assemble the partial workspaces from all MPI processes
 * @param partialWSName :: Name of the workspace to assemble
 * @param outputWSName :: Name of the assembled workspace (available in main
 * thread only)
 */
template <class Base>
Workspace_sptr
GenericDataProcessorAlgorithm<Base>::assemble(const std::string &partialWSName,
                                              const std::string &outputWSName) {
#ifdef MPI_BUILD
  std::string threadOutput = partialWSName;
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
  const std::string &threadOutput = partialWSName;

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
template <class Base>
void GenericDataProcessorAlgorithm<Base>::saveNexus(
    const std::string &outputWSName, const std::string &outputFile) {
  bool saveOutput = true;
#ifdef MPI_BUILD
  if (boost::mpi::communicator().rank() > 0)
    saveOutput = false;
#endif

  if (saveOutput && !outputFile.empty()) {
    IAlgorithm_sptr saveAlg = createChildAlgorithm("SaveNexus");
    saveAlg->setPropertyValue("Filename", outputFile);
    saveAlg->setPropertyValue("InputWorkspace", outputWSName);
    saveAlg->execute();
  }
}

/// Return true if we are running on the main thread
template <class Base> bool GenericDataProcessorAlgorithm<Base>::isMainThread() {
  bool mainThread;
#ifdef MPI_BUILD
  mainThread = (boost::mpi::communicator().rank() == 0);
#else
  mainThread = true;
#endif
  return mainThread;
}

/// Return the number of MPI processes running
template <class Base> int GenericDataProcessorAlgorithm<Base>::getNThreads() {
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
template <class Base>
Workspace_sptr
GenericDataProcessorAlgorithm<Base>::load(const std::string &inputData,
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
                       << world.size() << '\n';
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
 * data service, or create a new one. If the PropertyManager name is missing
 *(default) this will
 * look at m_propertyManagerPropertyName to get the correct value;
 *
 * @param propertyManager :: Name of the property manager to retrieve.
 */
template <class Base>
boost::shared_ptr<PropertyManager>
GenericDataProcessorAlgorithm<Base>::getProcessProperties(
    const std::string &propertyManager) const {
  std::string propertyManagerName(propertyManager);
  if (propertyManager.empty() && (!m_propertyManagerPropertyName.empty())) {
    if (!Base::existsProperty(m_propertyManagerPropertyName)) {
      std::stringstream msg;
      msg << "Failed to find property \"" << m_propertyManagerPropertyName
          << "\"";
      throw Exception::NotFoundError(msg.str(), this->name());
    }
    propertyManagerName = this->getPropertyValue(m_propertyManagerPropertyName);
  }

  boost::shared_ptr<PropertyManager> processProperties;
  if (PropertyManagerDataService::Instance().doesExist(propertyManagerName)) {
    processProperties =
        PropertyManagerDataService::Instance().retrieve(propertyManagerName);
  } else {
    Base::getLogger().notice() << "Could not find property manager\n";
    processProperties = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(propertyManagerName,
                                                        processProperties);
  }
  return processProperties;
}

template <class Base>
std::vector<std::string>
GenericDataProcessorAlgorithm<Base>::splitInput(const std::string &input) {
  UNUSED_ARG(input);
  throw std::runtime_error(
      "DataProcessorAlgorithm::splitInput is not implemented");
}

template <class Base>
void GenericDataProcessorAlgorithm<Base>::forwardProperties() {
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
template <class Base>
MatrixWorkspace_sptr
GenericDataProcessorAlgorithm<Base>::divide(const MatrixWorkspace_sptr lhs,
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
template <class Base>
MatrixWorkspace_sptr
GenericDataProcessorAlgorithm<Base>::divide(const MatrixWorkspace_sptr lhs,
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
template <class Base>
MatrixWorkspace_sptr
GenericDataProcessorAlgorithm<Base>::multiply(const MatrixWorkspace_sptr lhs,
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
template <class Base>
MatrixWorkspace_sptr
GenericDataProcessorAlgorithm<Base>::multiply(const MatrixWorkspace_sptr lhs,
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
template <class Base>
MatrixWorkspace_sptr
GenericDataProcessorAlgorithm<Base>::plus(const MatrixWorkspace_sptr lhs,
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
template <class Base>
MatrixWorkspace_sptr
GenericDataProcessorAlgorithm<Base>::plus(const MatrixWorkspace_sptr lhs,
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
template <class Base>
MatrixWorkspace_sptr
GenericDataProcessorAlgorithm<Base>::minus(const MatrixWorkspace_sptr lhs,
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
template <class Base>
MatrixWorkspace_sptr
GenericDataProcessorAlgorithm<Base>::minus(const MatrixWorkspace_sptr lhs,
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
template <class Base>
MatrixWorkspace_sptr
GenericDataProcessorAlgorithm<Base>::createWorkspaceSingleValue(
    const double &rhsValue) {
  MatrixWorkspace_sptr retVal =
      WorkspaceFactory::Instance().create("WorkspaceSingleValue", 1, 1, 1);
  retVal->dataY(0)[0] = rhsValue;

  return retVal;
}

template <typename T>
void GenericDataProcessorAlgorithm<T>::visualStudioC4661Workaround() {}

template class GenericDataProcessorAlgorithm<Algorithm>;
template class MANTID_API_DLL GenericDataProcessorAlgorithm<SerialAlgorithm>;
template class MANTID_API_DLL GenericDataProcessorAlgorithm<ParallelAlgorithm>;
template class MANTID_API_DLL
    GenericDataProcessorAlgorithm<DistributedAlgorithm>;

template <>
MANTID_API_DLL void
GenericDataProcessorAlgorithm<Algorithm>::visualStudioC4661Workaround() {}

} // namespace API
} // namespace Mantid
