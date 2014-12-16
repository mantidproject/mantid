//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/MultiThreaded.h"

#include <Poco/ActiveResult.h>

#include <cstdarg>

#ifdef _WIN32
#include <winsock2.h>
#endif

#ifdef MPI_BUILD
#include <boost/mpi.hpp>
#endif

namespace Mantid
{
namespace API
{
  namespace
  {
    /// static logger
    Kernel::Logger g_log("FrameworkManager");
    /// Key that that defines the location of the framework plugins
    const char * PLUGINS_DIR_KEY = "plugins.directory";
  }

  /** This is a function called every time NeXuS raises an error.
   * This swallows the errors and outputs nothing.
   *
   * @param data :: data passed in NXMSetError (will be NULL)
   * @param text :: text of the error.
   */
  void NexusErrorFunction(void *data, char *text)
  {
    UNUSED_ARG(data);
    UNUSED_ARG(text);
    // Do nothing.
  }


/// Default constructor
FrameworkManagerImpl::FrameworkManagerImpl()
#ifdef MPI_BUILD
  : m_mpi_environment()
#endif
{
  // Mantid only understands English...
  setGlobalLocaleToAscii();
  // Setup memory allocation scheme
  Kernel::MemoryOptions::initAllocatorOptions();

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

#ifdef _MSC_VER
  // This causes the exponent to consist of two digits (Windows Visual Studio normally 3, Linux default 2), where two digits are not sufficient I presume it uses more
  _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

  g_log.notice() << Mantid::welcomeMessage() << std::endl;
  loadPluginsUsingKey(PLUGINS_DIR_KEY);
  disableNexusOutput();
  setNumOMPThreadsToConfigValue();

#ifdef MPI_BUILD
  g_log.notice() << "This MPI process is rank: " << boost::mpi::communicator().rank() << std::endl;
#endif

  g_log.debug() << "FrameworkManager created." << std::endl;

  int updateInstrumentDefinitions = 0;
  int retVal = Kernel::ConfigService::Instance().getValue("UpdateInstrumentDefinitions.OnStartup",updateInstrumentDefinitions);
  if ((retVal == 1) &&  (updateInstrumentDefinitions == 1))
  {
    UpdateInstrumentDefinitions();
  }
  else
  {
    g_log.information() << "Instrument updates disabled - cannot update instrument definitions." << std::endl;
  }

  // the algorithm will see if it should run
  SendStartupUsageInfo();

}

/// Destructor
FrameworkManagerImpl::~FrameworkManagerImpl()
{
}

/// Update instrument definitions from github
void FrameworkManagerImpl::UpdateInstrumentDefinitions()
{
  try
  {
    IAlgorithm* algDownloadInstrument = this->createAlgorithm("DownloadInstrument");
    algDownloadInstrument->setAlgStartupLogging(false);
    Poco::ActiveResult<bool> result = algDownloadInstrument->executeAsync();
  }
  catch (Kernel::Exception::NotFoundError &)
  {
      g_log.debug() << "DowndloadInstrument algorithm is not available - cannot update instrument definitions." << std::endl;
  }
}

/// Sends startup information about OS and Mantid version
void FrameworkManagerImpl::SendStartupUsageInfo()
{
  // see whether or not to send
  int sendStartupUsageInfo = 0;
  int retVal = Kernel::ConfigService::Instance().getValue(
      "usagereports.enabled", sendStartupUsageInfo);
  if ((retVal == 0) || (sendStartupUsageInfo == 0)) {
    return; // exit early
  }

  // do it
  try
  {
    IAlgorithm* algSendStartupUsage = this->createAlgorithm("SendUsage");
    algSendStartupUsage->setAlgStartupLogging(false);
    Poco::ActiveResult<bool> result = algSendStartupUsage->executeAsync();
  }
  catch (Kernel::Exception::NotFoundError &)
  {
      g_log.debug() << "SendUsage algorithm is not available - cannot update send usage information." << std::endl;
  }
}

/**
 * Load a set of plugins from the path pointed to by the given config key
 * @param key :: A string containing a key to lookup in the ConfigService
 */
void FrameworkManagerImpl::loadPluginsUsingKey(const std::string & key)
{
  Kernel::ConfigServiceImpl& config = Kernel::ConfigService::Instance();
  std::string pluginDir = config.getString(key);
  if (pluginDir.length() > 0)
  {
    g_log.debug("Loading libraries from \"" + pluginDir + "\"");
    Kernel::LibraryManager::Instance().OpenAllLibraries(pluginDir, false);
  }
  else
  {
    g_log.debug("No library directory found in key \"" + key + "\"");
  }
}

/**
 * Set the global locale for all C++ stream operations to use simple ASCII characters.
 * If the system supports it UTF-8 encoding will be used, otherwise the 
 * classic C locale is used
 */
void FrameworkManagerImpl::setGlobalLocaleToAscii()
{
  // This ensures that all subsequent stream operations interpret everything as simple
  // ASCII. On systems in the UK and US having this as the system default is not an issue.
  // However, systems that have their encoding set differently can see unexpected behavour when
  // translating from string->numeral values. One example is floating-point interpretation in 
  // German where a comma is used instead of a period.
  std::locale::global(std::locale::classic());
}

/// Silence NeXus output
void FrameworkManagerImpl::disableNexusOutput()
{
  NXMSetError(NULL, NexusErrorFunction);
}

/**
 * Set the number of OpenMP cores to use based on the config value
 */
void FrameworkManagerImpl::setNumOMPThreadsToConfigValue()
{
  // Set the number of threads to use for this process
  int maxCores(0);
  int retVal = Kernel::ConfigService::Instance().getValue("MultiThreaded.MaxCores", maxCores);
  if(retVal > 0 && maxCores > 0)
  {
    setNumOMPThreads(maxCores);
  }
}

/**
 * Set the number of OpenMP cores to use based on the config value
 * @param nthreads :: The maximum number of threads to use
 */
void FrameworkManagerImpl::setNumOMPThreads(const int nthreads)
{
  g_log.debug() << "Setting maximum number of threads to " << nthreads << "\n";
  PARALLEL_SET_NUM_THREADS(nthreads);
}

/**
 * Returns the number of OpenMP threads that will be used
 * @returns The number of OpenMP threads that will be used in the next parallel call
 */
int FrameworkManagerImpl::getNumOMPThreads() const
{
  return PARALLEL_GET_MAX_THREADS;
}

/** Clears all memory associated with the AlgorithmManager
 *  and with the Analysis & Instrument data services.
 */
void FrameworkManagerImpl::clear()
{
  clearAlgorithms();
  clearInstruments();
  clearData();
  clearPropertyManagers();
}

/**
 * Clear memory associated with the AlgorithmManager
 */
void FrameworkManagerImpl::clearAlgorithms()
{
  AlgorithmManager::Instance().clear();
}

/**
 * Clear memory associated with the ADS
 */
void FrameworkManagerImpl::clearData()
{
  AnalysisDataService::Instance().clear();
  Mantid::API::MemoryManager::Instance().releaseFreeMemory();
}

/**
 * Clear memory associated with the IDS
 */
void FrameworkManagerImpl::clearInstruments()
{
  InstrumentDataService::Instance().clear();
}

/**
 * Clear memory associated with the PropertyManagers
 */
void FrameworkManagerImpl::clearPropertyManagers()
{
  PropertyManagerDataService::Instance().clear();
}

/** Creates and initialises an instance of an algorithm
 * 
 *  @param algName :: The name of the algorithm required
 *  @param version :: The version of the algorithm
 *  @return A pointer to the created algorithm.
 *          WARNING! DO NOT DELETE THIS POINTER, because it is owned
 *          by a shared pointer in the AlgorithmManager.
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 */
IAlgorithm* FrameworkManagerImpl::createAlgorithm(const std::string& algName, const int& version)
{ 
   IAlgorithm* alg = AlgorithmManager::Instance().create(algName,version).get();
   return alg;
}

/** Creates an instance of an algorithm and sets the properties provided
 * 
 *  @param algName :: The name of the algorithm required
 *  @param propertiesArray :: A single string containing properties in the 
 *                         form "Property1=Value1;Property2=Value2;..."
 *  @param version :: The version of the algorithm
 *  @return A pointer to the created algorithm
 *          WARNING! DO NOT DELETE THIS POINTER, because it is owned
 *          by a shared pointer in the AlgorithmManager.
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 *  @throw std::invalid_argument Thrown if properties string is ill-formed
 */ 
IAlgorithm* FrameworkManagerImpl::createAlgorithm(const std::string& algName,const std::string& propertiesArray, const int& version)
{
  // Use the previous method to create the algorithm
  IAlgorithm *alg = AlgorithmManager::Instance().create(algName,version).get();//createAlgorithm(algName);
  alg->setProperties(propertiesArray);
  return alg;
}

/** Creates an instance of an algorithm, sets the properties provided and
 *       then executes it.
 * 
 *  @param algName :: The name of the algorithm required
 *  @param propertiesArray :: A single string containing properties in the 
 *                         form "Property1=Value1;Property2=Value2;..."
 *  @param version :: The version of the algorithm
 *  @return A pointer to the executed algorithm
 *          WARNING! DO NOT DELETE THIS POINTER, because it is owned
 *          by a shared pointer in the AlgorithmManager.
 * 
 *  @throw NotFoundError Thrown if algorithm requested is not registered
 *  @throw std::invalid_argument Thrown if properties string is ill-formed
 *  @throw runtime_error Thrown if algorithm cannot be executed
 */ 
IAlgorithm* FrameworkManagerImpl::exec(const std::string& algName, const std::string& propertiesArray, const int& version)
{
  // Make use of the previous method for algorithm creation and property setting
  IAlgorithm *alg = createAlgorithm(algName, propertiesArray,version);
  
  // Now execute the algorithm
  alg->execute();
  
  return alg;
}


/** Run any algorithm with a variable number of parameters
 *
 * @param algorithmName
 * @param count :: number of arguments given.
 * @return the algorithm created
 */
IAlgorithm_sptr FrameworkManagerImpl::exec(const std::string& algorithmName, int count, ...)
{
  if (count % 2 == 1)
  {
    throw std::runtime_error("Must have an even number of parameter/value string arguments");
  }

 // Create the algorithm
  IAlgorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(algorithmName, -1);
  alg->initialize();
  if (!alg->isInitialized())
    throw std::runtime_error(algorithmName + " was not initialized.");


  va_list Params;
  va_start(Params, count);
  for(int i = 0; i < count; i += 2 )
  {
    std::string paramName = va_arg(Params, const char *);
    std::string paramValue = va_arg(Params, const char *);
    alg->setPropertyValue(paramName, paramValue);
  }
  va_end(Params);

  alg->execute();
  return alg;
}


/** Returns a shared pointer to the workspace requested
 * 
 *  @param wsName :: The name of the workspace
 *  @return A pointer to the workspace
 * 
 *  @throw NotFoundError If workspace is not registered with analysis data service
 */
Workspace* FrameworkManagerImpl::getWorkspace(const std::string& wsName)
{
  Workspace *space;
  try
  {
    space = AnalysisDataService::Instance().retrieve(wsName).get();
  }
  catch (Kernel::Exception::NotFoundError&)
  {
    throw Kernel::Exception::NotFoundError("Unable to retrieve workspace",wsName);
  }
  return space;
}

/** Removes and deletes a workspace from the data service store.
 * 
 *  @param wsName :: The user-given name for the workspace 
 *  @return true if the workspace was found and deleted
 * 
 *  @throw NotFoundError Thrown if workspace cannot be found
 */
bool FrameworkManagerImpl::deleteWorkspace(const std::string& wsName)
{
  bool retVal = false;
  boost::shared_ptr<Workspace> ws_sptr;
  try
  {
    ws_sptr = AnalysisDataService::Instance().retrieve(wsName);
  }
  catch(Kernel::Exception::NotFoundError&ex)
  {
    g_log.error() << ex.what() << std::endl;
    return false;
  }

  boost::shared_ptr<WorkspaceGroup> ws_grpsptr=boost::dynamic_pointer_cast<WorkspaceGroup>(ws_sptr);
  if(ws_grpsptr)
  {
    //  selected workspace is a group workspace
      AnalysisDataService::Instance().deepRemoveGroup( wsName );
  }
  // Make sure we drop the references so the memory will get freed when we expect it to
  ws_sptr.reset();
  ws_grpsptr.reset();
  try
  {
    AnalysisDataService::Instance().remove(wsName);
    retVal = true;
  }
  catch (Kernel::Exception::NotFoundError&)
  {
    //workspace was not found
    g_log.error()<<"Workspace "<< wsName << " could not be found."<<std::endl;
    retVal = false;
  }
  Mantid::API::MemoryManager::Instance().releaseFreeMemory();
  return retVal;
}

} // namespace API
} // Namespace Mantid
