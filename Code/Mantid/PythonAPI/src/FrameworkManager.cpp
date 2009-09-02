//---------------------------------------
// Includes
//------------------------------------
#include "MantidPythonAPI/FrameworkManager.h"

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
Mantid::Kernel::Logger& FrameworkManager::g_log = Mantid::Kernel::Logger::get("MantidPython");

/// Default constructor
FrameworkManager::FrameworkManager()
{
	API::FrameworkManager::Instance();
}

/// Clear the FrameworkManager	
void FrameworkManager::clear()
{
	API::FrameworkManager::Instance().clear();
}

/**
 * Clear memory associated with the AlgorithmManager
 */
void FrameworkManager::clearAlgorithms()
{
  API::FrameworkManager::Instance().clearAlgorithms();
}

/**
 * Clear memory associated with the ADS
 */
void FrameworkManager::clearData()
{
  API::FrameworkManager::Instance().clearData();
}

/**
 * Clear memory associated with the IDS
 */
void FrameworkManager::clearInstruments()
{
  API::FrameworkManager::Instance().clearInstruments();
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
  * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName)
{
	return API::FrameworkManager::Instance().createAlgorithm(algName);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param version :: The version of the algorithm to use.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName, const int& version)
{
	return API::FrameworkManager::Instance().createAlgorithm(algName, version);
}

/**
 * Creates a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
  * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName, const std::string& propertiesArray)
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
API::IAlgorithm* FrameworkManager::createAlgorithm(const std::string& algName, const std::string& propertiesArray,const int& version)
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
API::IAlgorithm* FrameworkManager::execute(const std::string& algName, const std::string& propertiesArray,const int& version)
{
	return API::FrameworkManager::Instance().exec(algName, propertiesArray, version);
}

/**
 * Creates and executes a specified algorithm.
 * \param algName :: The name of the algorithm to execute.
 * \param propertiesArray :: A separated string containing the properties and their values.
 * \return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManager::execute(const std::string& algName, const std::string& propertiesArray)
{
	return API::FrameworkManager::Instance().exec(algName, propertiesArray);
}

/**
 * Returns a specified MatrixWorkspace.
 * \param wsName :: The name of the workspace to retrieve.
 * \return Shared pointer to workspace.
 **/
API::MatrixWorkspace* FrameworkManager::getMatrixWorkspace(const std::string& wsName)
{
  API::MatrixWorkspace *mtx_wksp = dynamic_cast<API::MatrixWorkspace*>( API::FrameworkManager::Instance().getWorkspace(wsName) );
  if( !mtx_wksp )
  {
    throw std::runtime_error("\"" + wsName + "\" is not a matrix workspace. This may be a table workspace, try getTableWorkspace().");
  }
  //Normal path
  return mtx_wksp;
}

/**
* Returns a specified TableWorkspace.
* @param wsName :: The name of the workspace to retrieve.
* @return Shared pointer to workspace.
**/
API::ITableWorkspace* FrameworkManager::getTableWorkspace(const std::string& wsName)
{
  API::ITableWorkspace *tbl_wksp = dynamic_cast<API::ITableWorkspace*>( API::FrameworkManager::Instance().getWorkspace(wsName) );
  if( !tbl_wksp )
  {
    throw std::runtime_error("\"" + wsName + "\" is not a table workspace. This may be a matrix workspace, try getMatrixWorkspace().");
  }
  return tbl_wksp;
}

/**
 * Return a list of pointers to all of the workspaces in a specified workspace group
 * @param group_name The name of the group
 * @return A vector of pointers to API::Workspace objects
 */
std::vector<API::MatrixWorkspace*> FrameworkManager::getMatrixWorkspaceGroup(const std::string& group_name)
{
  API::WorkspaceGroup* ws_group = 
    dynamic_cast<API::WorkspaceGroup*>( API::FrameworkManager::Instance().getWorkspace(group_name) );
  std::vector<API::MatrixWorkspace*> group_ptrs; 
  if( ws_group )
  {
    const std::vector<std::string> & ws_names = ws_group->getNames();
    std::vector<std::string>::const_iterator iend = ws_names.end();
    for( std::vector<std::string>::const_iterator itr = ws_names.begin(); itr != iend;
	 ++itr )
    {
      API::MatrixWorkspace *mtx_wksp = 
	dynamic_cast<API::MatrixWorkspace*>(API::FrameworkManager::Instance().getWorkspace(*itr));
      if( mtx_wksp ) group_ptrs.push_back(mtx_wksp);
    }
  }
  return group_ptrs;
}

/**
 * Deletes a specified workspace.
 * \param wsName :: The name of the workspace to delete.
 * \return Boolean result.
 **/
bool FrameworkManager::deleteWorkspace(const std::string& wsName)
{
	return API::FrameworkManager::Instance().deleteWorkspace(wsName);
}

/**
 * Returns the name of all the workspaces.
 * \return Vector of strings.
 **/
std::vector<std::string> FrameworkManager::getWorkspaceNames() const
{
  return API::AnalysisDataService::Instance().getObjectNames();
}

/**
 * Returns the names of all the workspace groups
 * \return A vector of strings.
 **/
std::vector<std::string> FrameworkManager::getWorkspaceGroupNames() const
{
  std::vector<std::string> ws_names = getWorkspaceNames();
  std::vector<std::string> grp_names;
  std::vector<std::string>::const_iterator iend = ws_names.end();
  for( std::vector<std::string>::const_iterator itr = ws_names.begin(); itr != iend;
       ++itr )
  {
    API::Workspace *wksp =  API::FrameworkManager::Instance().getWorkspace(*itr);
    if( dynamic_cast<API::WorkspaceGroup*>(wksp) )
    {
      grp_names.push_back(*itr);
    }

  }
  return grp_names;
}

/**
 * Get the names within a workspace group
 * @param group_name The name of the group
 */
std::vector<std::string> FrameworkManager::getWorkspaceGroupEntries(const std::string & group_name) const
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
std::vector<std::string> FrameworkManager::getAlgorithmNames() const
{
  return API::AlgorithmFactory::Instance().getKeys();
}

/**
  * Create the simple Python API module
  * @param gui Whether the module is being made for use with qtiplot or not
  **/
void FrameworkManager::createPythonSimpleAPI(bool gui)
{
  //Redirect to static helper class
  SimplePythonAPI::createModule(gui);
}

/**
 * Send a log message to Mantid
 * @param msg The log message
 */
void FrameworkManager::sendLogMessage(const std::string & msg) 
{
  g_log.notice(msg); 
}
	
/**
 * Adds a algorithm created in Python to Mantid's algorithms.
 * Converts the Python object to a C++ object - not sure how, will find out.
 * \param pyAlg :: The Python based algorithm to add.
 * \returns The number of Python algorithms in Mantid
 **/
int FrameworkManager::addPythonAlgorithm(PyObject* pyAlg)
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
void FrameworkManager::executePythonAlgorithm(std::string algName)
{
	API::AlgorithmFactory::Instance().executePythonAlg(algName);
}


}
}

