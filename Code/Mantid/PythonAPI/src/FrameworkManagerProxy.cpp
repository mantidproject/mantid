//---------------------------------------
// Includes
//------------------------------------
#include "MantidPythonAPI/FrameworkManagerProxy.h"

#include <boost/python/handle.hpp>
#include <boost/python/extract.hpp>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidPythonAPI/PyAlgorithm.h"
#include "MantidPythonAPI/SimplePythonAPI.h"
#include "MantidAPI/FrameworkManager.h"

namespace Mantid
{
namespace PythonAPI
{

// Initialize the logger
Mantid::Kernel::Logger& FrameworkManagerProxy::g_log = Mantid::Kernel::Logger::get("MantidPython");

/// Default constructor
FrameworkManagerProxy::FrameworkManagerProxy() 
  : m_delete_observer(*this, &FrameworkManagerProxy::deleteNotificationReceived),
    m_add_observer(*this, &FrameworkManagerProxy::addNotificationReceived),
    m_replace_observer(*this, &FrameworkManagerProxy::replaceNotificationReceived)
{
  API::FrameworkManager::Instance();
  API::AnalysisDataService::Instance().notificationCenter.addObserver(m_delete_observer);
  API::AnalysisDataService::Instance().notificationCenter.addObserver(m_add_observer);
  API::AnalysisDataService::Instance().notificationCenter.addObserver(m_replace_observer);
}

///Destructor
FrameworkManagerProxy::~FrameworkManagerProxy()
{
  API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_replace_observer);
  API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_add_observer);
  API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_delete_observer);
}

/// Clear the FrameworkManager	
void FrameworkManagerProxy::clear()
{
  API::FrameworkManager::Instance().clear();
}

/**
 * Clear memory associated with the AlgorithmManager
 */
void FrameworkManagerProxy::clearAlgorithms()
{
  API::FrameworkManager::Instance().clearAlgorithms();
}

/**
 * Clear memory associated with the ADS
 */
void FrameworkManagerProxy::clearData()
{
  API::FrameworkManager::Instance().clearData();
}

/**
 * Clear memory associated with the IDS
 */
void FrameworkManagerProxy::clearInstruments()
{
  API::FrameworkManager::Instance().clearInstruments();
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
  * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::createAlgorithm(const std::string& algName)
{
	return API::FrameworkManager::Instance().createAlgorithm(algName);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param version :: The version of the algorithm to use.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::createAlgorithm(const std::string& algName, const int& version)
{
	return API::FrameworkManager::Instance().createAlgorithm(algName, version);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
  * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::createAlgorithm(const std::string& algName, const std::string& propertiesArray)
{
	return API::FrameworkManager::Instance().createAlgorithm(algName, propertiesArray);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
 * \param version :: The version of the algorithm to use.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::createAlgorithm(const std::string& algName, const std::string& propertiesArray,const int& version)
{
	return API::FrameworkManager::Instance().createAlgorithm(algName, propertiesArray, version);
}

/**
 * Creates and executes a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
 * \param version :: The version of the algorithm to use.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::execute(const std::string& algName, const std::string& propertiesArray,const int& version)
{
	return API::FrameworkManager::Instance().exec(algName, propertiesArray, version);
}

/**
 * Creates and executes a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::execute(const std::string& algName, const std::string& propertiesArray)
{
	return API::FrameworkManager::Instance().exec(algName, propertiesArray);
}

/**
 * Returns a specified MatrixWorkspace.
 * \param wsName :: The name of the workspace to retrieve.
 * \return Shared pointer to workspace.
 **/
boost::shared_ptr<API::MatrixWorkspace> FrameworkManagerProxy::retrieveMatrixWorkspace(const std::string& wsName)
{
  API::MatrixWorkspace_sptr matrix_wksp =  boost::dynamic_pointer_cast<API::MatrixWorkspace>(retrieveWorkspace(wsName));
  if( matrix_wksp.get() )
  {
    return matrix_wksp;
  }
  else
  {
    throw std::runtime_error("\"" + wsName + "\" is not a matrix workspace. "
			     "This may be a table workspace, try getTableWorkspace().");
  }
}

/**
* Returns a specified TableWorkspace.
* @param wsName :: The name of the workspace to retrieve.
* @return Shared pointer to workspace.
**/
boost::shared_ptr<API::ITableWorkspace> FrameworkManagerProxy::retrieveTableWorkspace(const std::string& wsName)
{
  API::ITableWorkspace_sptr table_wksp =  boost::dynamic_pointer_cast<API::ITableWorkspace>(retrieveWorkspace(wsName));
  if( table_wksp.get() )
  {
    return table_wksp;
  }
  else
  {
    throw std::runtime_error("\"" + wsName + "\" is not a matrix workspace. "
			     "This may be a table workspace, try getTableWorkspace().");
  }
}

/**
 * Return a pointer to a WorkspaceGroup
 * @param group_name The name of the group
 * @return A pointer to API::WorkspaceGroup object
 */
boost::shared_ptr<API::WorkspaceGroup> FrameworkManagerProxy::retrieveWorkspaceGroup(const std::string& group_name)
{
  API::WorkspaceGroup_sptr wksp_group = boost::dynamic_pointer_cast<API::WorkspaceGroup>(retrieveWorkspace(group_name));
  if( wksp_group.get() )
  {
    return wksp_group;
  }
  else
  {
    throw std::runtime_error("\"" + group_name + "\" is not a group workspace. "
			     "This may be a matrix workspace, try getMatrixWorkspace().");
  }
}

/**
 * Deletes a specified workspace.
 * \param wsName :: The name of the workspace to delete.
 * \return Boolean result.
 **/
bool FrameworkManagerProxy::deleteWorkspace(const std::string& wsName)
{
  return API::FrameworkManager::Instance().deleteWorkspace(wsName);
}

/**
 * Returns the name of all the workspaces.
 * \return Vector of strings.
 **/
std::set<std::string> FrameworkManagerProxy::getWorkspaceNames() const
{
  return API::AnalysisDataService::Instance().getObjectNames();
}

/**
 * Returns the names of all the workspace groups
 * \return A vector of strings.
 **/
std::set<std::string> FrameworkManagerProxy::getWorkspaceGroupNames() const
{
  std::set<std::string> ws_names = getWorkspaceNames();
  std::set<std::string> grp_names;
  std::set<std::string>::const_iterator iend = ws_names.end();
  for( std::set<std::string>::const_iterator itr = ws_names.begin(); itr != iend;
       ++itr )
  {
    API::Workspace *wksp =  API::FrameworkManager::Instance().getWorkspace(*itr);
    if( dynamic_cast<API::WorkspaceGroup*>(wksp) )
    {
      grp_names.insert(*itr);
    }

  }
  return grp_names;
}

/**
 * Get the names within a workspace group
 * @param group_name The name of the group
 */
std::vector<std::string> FrameworkManagerProxy::getWorkspaceGroupEntries(const std::string & group_name) const
{
  API::WorkspaceGroup* ws_group = 
    dynamic_cast<API::WorkspaceGroup*>( API::FrameworkManager::Instance().getWorkspace(group_name) );
  std::vector<std::string> entries;
  if( ws_group )
  {
    entries = ws_group->getNames();
  }
  return entries;
}

/**
 * Returns the name of all the algorithms.
 * \return Vector of strings.
 **/
std::vector<std::string> FrameworkManagerProxy::getAlgorithmNames() const
{
  return API::AlgorithmFactory::Instance().getKeys();
}

/**
  * Create the simple Python API module
  * @param gui Whether the module is being made for use with qtiplot or not
  **/
void FrameworkManagerProxy::createPythonSimpleAPI(bool gui)
{
  //Redirect to static helper class
  SimplePythonAPI::createModule(gui);
}

/**
 * Send a log message to Mantid
 * @param msg The log message
 */
void FrameworkManagerProxy::sendLogMessage(const std::string & msg) 
{
  g_log.notice(msg); 
}

/**
 * Check if a given workspace name exists in the ADS 
 * @param name A string specifying a name to check
 */
bool FrameworkManagerProxy::workspaceExists(const std::string & name) const
{
  return API::AnalysisDataService::Instance().doesExist(name);
}

/// Called when a workspace is removed.
void FrameworkManagerProxy::workspaceRemoved(const std::string &)
{
}

/// Called when a workspace is added.
void FrameworkManagerProxy::workspaceAdded(const std::string &)
{
}

/// Called when a workspace is replaced.
void FrameworkManagerProxy::workspaceReplaced(const std::string &)
{
}

/**
 * Adds a algorithm created in Python to Mantid's algorithms.
 * Converts the Python object to a C++ object - not sure how, will find out.
 * \param pyAlg :: The Python based algorithm to add.
 * \returns The number of Python algorithms in Mantid
 **/
int FrameworkManagerProxy::addPythonAlgorithm(PyObject* pyAlg)
{
  boost::python::handle<> ph(boost::python::borrowed(pyAlg));
  PyAlgorithm* alg = boost::python::extract<PyAlgorithm*>(boost::python::object(ph));
  API::AlgorithmFactory::Instance().addPyAlgorithm(alg);
  return API::AlgorithmFactory::Instance().numPythonAlgs();
}


/**
 * Execute one of the Python algorithms that has been added to Mantid.
 * \param algName :: The name of the algorithm to run.
 **/
void FrameworkManagerProxy::executePythonAlgorithm(std::string algName)
{
  API::AlgorithmFactory::Instance().executePythonAlg(algName);
}


//--------------------------------------------------------------------------
//
// Private member functions
//
//--------------------------------------------------------------------------
/**
 * Get a workspace pointer from the ADS
 * @param wsName The name of the workspace to retrieve, throws if the pointer is invalid
 */
boost::shared_ptr<Mantid::API::Workspace> FrameworkManagerProxy::retrieveWorkspace(const std::string & wsName)
{
  try
  {
    API::Workspace_sptr wksp = API::AnalysisDataService::Instance().retrieve(wsName);
    return wksp;
  }
  catch(Mantid::Kernel::Exception::NotFoundError &)
  {
    throw std::runtime_error("Workspace \"" + wsName + "\" not found.");
  }
}

/**
 * Utility function called when a workspace is deleted within the service
 * @param notice A pointer to a WorkspaceDeleteNotification object
 */
void FrameworkManagerProxy::deleteNotificationReceived(Mantid::API::WorkspaceDeleteNotification_ptr notice)
{
  /// This function may be overridden in Python
  workspaceRemoved(notice->object_name());  
}

/**
 * Utility function called when a workspace is added within the service
 * @param notice A pointer to a WorkspaceDeleteNotification object
 */
void FrameworkManagerProxy::addNotificationReceived(Mantid::API::WorkspaceAddNotification_ptr notice)
{  
}

/**
 * Utility function called when a workspace is replaced within the service
 * @param notice A pointer to a WorkspaceAfterReplaceNotification object
 */
void FrameworkManagerProxy::replaceNotificationReceived(Mantid::API::WorkspaceAfterReplaceNotification_ptr notice)

{  
  /// This function may be overridden in Python
  workspaceReplaced(notice->object_name());
}

}
}

