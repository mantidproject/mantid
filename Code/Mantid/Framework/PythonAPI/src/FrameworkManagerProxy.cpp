//---------------------------------------
// Includes
//------------------------------------
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Strings.h"
#include "MantidPythonAPI/FrameworkManagerProxy.h"
#include "MantidPythonAPI/PyAlgorithmWrapper.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"

using Mantid::API::AlgorithmManager;
using Mantid::API::IPeaksWorkspace;
using Mantid::API::IPeaksWorkspace_sptr;

namespace Mantid
{
namespace PythonAPI
{

typedef std::vector<Mantid::Kernel::Property*> PropertyVector;
typedef std::vector<std::string> StringVector;


// Initialize the logger
Mantid::Kernel::Logger& FrameworkManagerProxy::g_log = Mantid::Kernel::Logger::get("MantidPython");

/// Default constructor
FrameworkManagerProxy::FrameworkManagerProxy() 
  : m_delete_observer(*this, &FrameworkManagerProxy::deleteNotificationReceived),
    m_add_observer(*this, &FrameworkManagerProxy::addNotificationReceived),
    m_replace_observer(*this, &FrameworkManagerProxy::replaceNotificationReceived),
    m_clear_observer(*this, &FrameworkManagerProxy::clearNotificationReceived),
    m_algupdate_observer(*this, &FrameworkManagerProxy::handleAlgorithmFactoryUpdate)
{
  API::FrameworkManager::Instance();

  API::AnalysisDataService::Instance().notificationCenter.addObserver(m_delete_observer);
  API::AnalysisDataService::Instance().notificationCenter.addObserver(m_replace_observer);
  API::AnalysisDataService::Instance().notificationCenter.addObserver(m_clear_observer);
}

///Destructor
FrameworkManagerProxy::~FrameworkManagerProxy()
{
  API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_clear_observer);
  API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_replace_observer);
  API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_delete_observer);
  API::AlgorithmFactory::Instance().notificationCenter.removeObserver(m_algupdate_observer);
}

void FrameworkManagerProxy::observeAlgFactoryUpdates(bool listen, bool force_update)
{
  if( listen )
  {
    API::AlgorithmFactory::Instance().notificationCenter.addObserver(m_algupdate_observer);
    if( force_update )
    {
      API::AlgorithmFactory::Instance().notificationCenter.postNotification(new API::AlgorithmFactoryUpdateNotification);
    }
  }
  else
  {
    API::AlgorithmFactory::Instance().notificationCenter.removeObserver(m_algupdate_observer);
  }
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

std::vector<std::string> FrameworkManagerProxy::getRegisteredAlgorithms(bool includeHidden)
{
  return API::AlgorithmFactory::Instance().getKeys(includeHidden);
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
 * Return a string property from the ConfigService
 * @param key :: the key of the config value you require
 * @returns the value of the config item you require
 */
std::string FrameworkManagerProxy::getConfigProperty(const std::string & key) const
{
  return Kernel::ConfigService::Instance().getString(key);
}

/**
 * Check whether a given name is an algorithm using a case-insensitive search
 * @param testName :: The name to test
 * @returns The algorithm name in the correct case or an empty string if there is no
 * algorithm of this name.
 */
std::string FrameworkManagerProxy::isAlgorithmName(std::string testName) const
{
  std::transform(testName.begin(), testName.end(), testName.begin(), tolower);
  const std::vector<std::string> keys = API::AlgorithmFactory::Instance().getKeys();
  const std::size_t numKeys = keys.size();
  for(std::size_t i = 0; i < numKeys; ++i)
  {
    std::string key = keys[i];
    const std::string name = key.substr(0, key.find_last_of('|'));
    std::string lowered(name.size(), ' ');
    std::transform(name.begin(), name.end(), lowered.begin(), tolower);
    if( lowered == testName ) return name;
  }

  return "";
}

/**
 * Creates a managed version of a specified algorithm.
 * @param algName :: The name of the algorithm to execute.
 * @param version :: The version number (default=-1=highest version).
 * @return Pointer to algorithm.
 **/
API::IAlgorithm* FrameworkManagerProxy::createManagedAlgorithm(const std::string& algName, const int version)
{
  return API::FrameworkManager::Instance().createAlgorithm(algName, version);
}

/**
 * Creates a managed version of a specified algorithm.
 * @param algName :: The name of the algorithm to execute.
 * @param version :: The version number (default=-1=highest version).
 * @return Pointer to algorithm.
 **/
API::IAlgorithm_sptr FrameworkManagerProxy::createUnmanagedAlgorithm(const std::string& algName, const int version)
{
  API::IAlgorithm_sptr alg = API::AlgorithmManager::Instance().createUnmanaged(algName, version);
  alg->initialize();
  return alg;
}

/** Returns the deprecation message (if any) for deprecated algorithms.
 *
 * @param algName :: The name of the algorithm to execute.
 * @param version :: The version number (default=-1=highest version).
 * @return string, empty if algo is NOT deprecated.
 **/
std::string FrameworkManagerProxy::algorithmDeprecationMessage(const std::string& algName, int version)
{
  std::string deprecMessage = "";
  API::Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(algName, version);
  API::DeprecatedAlgorithm * depr = dynamic_cast<API::DeprecatedAlgorithm *>(alg.get());
  if (depr)
    deprecMessage = depr->deprecationMsg(alg.get());
  return deprecMessage;
}

/**
 * Return a vector of strings containing the properties in their correct order
 * @param algm :: A pointer to the algorithm
 * @returns A pointer to a vector of strings
 */
std::vector<std::string> * FrameworkManagerProxy::getPropertyOrder(const API::IAlgorithm * algm)
{
  if (algm == NULL)
    throw std::runtime_error("Encountered NULL algorithm pointer in FrameworkManagerProxy::getPropertyOrder");

  // get a sorted copy of the properties
  PropertyVector properties(algm->getProperties());
  std::sort(properties.begin(), properties.end(), PropertyOrdering());

  // generate the sanitized names
  std::vector<std::string>* names = new std::vector<std::string>();
  names->reserve(properties.size());
  size_t numProps = properties.size();
  for ( size_t i = 0; i < numProps; ++i)
  {
    names->push_back(removeCharacters(properties[i]->name(), ""));
  }
  return names;
}

std::string FrameworkManagerProxy::createAlgorithmDocs(const std::string& algName, const int version)
{
  const std::string EOL="\n";
  API::IAlgorithm_sptr algm = API::AlgorithmManager::Instance().createUnmanaged(algName, version);
  algm->initialize();

  // Put in the quick overview message
  std::stringstream buffer;
  std::string temp = algm->getOptionalMessage();
  if (temp.size() > 0)
    buffer << temp << EOL << EOL;

  // get a sorted copy of the properties
  PropertyVector properties(algm->getProperties());
  std::sort(properties.begin(), properties.end(), PropertyOrdering());

  // generate the sanitized names
  StringVector names(properties.size());
  size_t numProps = properties.size();
  for ( size_t i = 0; i < numProps; ++i) 
  {
    names[i] = removeCharacters(properties[i]->name(), "");
  }

  buffer << "Property descriptions: " << EOL << EOL;
  // write the actual property descriptions
  Mantid::Kernel::Property *prop;
  for ( size_t i = 0; i < numProps; ++i) 
  {
    prop = properties[i];
    buffer << names[i] << "("
           << Mantid::Kernel::Direction::asText(prop->direction());
    if (!prop->isValid().empty())
      buffer << ":req";
    buffer << ") *" << prop->type() << "* ";
    std::set<std::string> allowed = prop->allowedValues();
    if (!prop->documentation().empty() || !allowed.empty())
    {
      buffer << "      " << prop->documentation();
      if (!allowed.empty())
      {
        buffer << " [" << Kernel::Strings::join(allowed.begin(), allowed.end(), ", ");
        buffer << "]";
      }
      buffer << EOL;
      if( i < numProps - 1 ) buffer << EOL;
    }
  }

  return buffer.str();
}

/**
 * Returns a specified MatrixWorkspace.
 * @param wsName :: The name of the workspace to retrieve.
 * @return Shared pointer to workspace.
 * @throw runtime_error if not of the right type
 **/
boost::shared_ptr<API::MatrixWorkspace> FrameworkManagerProxy::retrieveMatrixWorkspace(const std::string& wsName)
{
  return boost::dynamic_pointer_cast<API::MatrixWorkspace>(retrieveWorkspace(wsName));
//  API::MatrixWorkspace_sptr matrix_wksp =  boost::dynamic_pointer_cast<API::MatrixWorkspace>(retrieveWorkspace(wsName));
//  if( matrix_wksp.get() )
//  {
//    return matrix_wksp;
//  }
//  else
//  {
//    throw std::runtime_error("\"" + wsName + "\" is not a matrix workspace. "
//        "This may be a table workspace, try getTableWorkspace().");
//  }
}

/** Return pointer to IEventWorkspace
 * @param wsName :: The name of the workspace to retrieve.
 * @return Shared pointer to workspace.
 * @throw runtime_error if not of the right type
 */
boost::shared_ptr<API::IEventWorkspace> FrameworkManagerProxy::retrieveIEventWorkspace(const std::string& wsName)
{
  return boost::dynamic_pointer_cast<API::IEventWorkspace>(retrieveWorkspace(wsName));
//  API::IEventWorkspace_sptr event = boost::dynamic_pointer_cast<API::IEventWorkspace>(retrieveWorkspace(wsName));
//  if (event != NULL)
//  {
//    return event;
//  }
//  else
//  {
//    throw std::runtime_error("\"" + wsName + "\" is not an event workspace. ");
//  }
}

/** Return pointer to IPeaksWorkspace
 * @param wsName :: The name of the workspace to retrieve.
 * @return Shared pointer to workspace.
 * @throw runtime_error if not of the right type
 */
boost::shared_ptr<IPeaksWorkspace> FrameworkManagerProxy::retrieveIPeaksWorkspace(const std::string& wsName)
{
  return  boost::dynamic_pointer_cast<IPeaksWorkspace>(retrieveWorkspace(wsName));

//  IPeaksWorkspace_sptr ws = boost::dynamic_pointer_cast<IPeaksWorkspace>(retrieveWorkspace(wsName));
//  if (ws != NULL)
//    return ws;
//  else
//    throw std::runtime_error("\"" + wsName + "\" is not a peaks workspace. ");
}

/** Return pointer to IMDWorkspace
 * @param wsName :: The name of the workspace to retrieve.
 * @return Shared pointer to workspace.
 * @throw runtime_error if not of the right type
 */
boost::shared_ptr<API::IMDWorkspace> FrameworkManagerProxy::retrieveIMDWorkspace(const std::string& wsName)
{
  return boost::dynamic_pointer_cast<API::IMDWorkspace>(retrieveWorkspace(wsName));
//  API::IMDWorkspace_sptr ws = boost::dynamic_pointer_cast<API::IMDWorkspace>(retrieveWorkspace(wsName));
//  if (ws != NULL)
//  {
//    return ws;
//  }
//  else
//  {
//    throw std::runtime_error("\"" + wsName + "\" is not an MD workspace. ");
//  }
}


/** Return pointer to IMDHistoWorkspace
 * @param wsName :: The name of the workspace to retrieve.
 * @return Shared pointer to workspace.
 * @throw runtime_error if not of the right type
 */
boost::shared_ptr<API::IMDHistoWorkspace> FrameworkManagerProxy::retrieveIMDHistoWorkspace(const std::string& wsName)
{
  return boost::dynamic_pointer_cast<API::IMDHistoWorkspace>(retrieveWorkspace(wsName));
//  API::IMDHistoWorkspace_sptr ws = boost::dynamic_pointer_cast<API::IMDHistoWorkspace>(retrieveWorkspace(wsName));
//  if (ws != NULL)
//  {
//    return ws;
//  }
//  else
//  {
//    throw std::runtime_error("\"" + wsName + "\" is not an MDHistoWorkspace. ");
//  }
}



/** Return pointer to IMDEventWorkspace
 * @param wsName :: The name of the workspace to retrieve.
 * @return Shared pointer to workspace.
 * @throw runtime_error if not of the right type
 */
boost::shared_ptr<API::IMDEventWorkspace> FrameworkManagerProxy::retrieveIMDEventWorkspace(const std::string& wsName)
{
  return boost::dynamic_pointer_cast<API::IMDEventWorkspace>(retrieveWorkspace(wsName));
//  API::IMDEventWorkspace_sptr ws = boost::dynamic_pointer_cast<API::IMDEventWorkspace>(retrieveWorkspace(wsName));
//  if (ws != NULL)
//  {
//    return ws;
//  }
//  else
//  {
//    throw std::runtime_error("\"" + wsName + "\" is not an MD Event Workspace. ");
//  }
}


/**
* Returns a specified TableWorkspace.
* @param wsName :: The name of the workspace to retrieve.
* @return Shared pointer to workspace.
**/
boost::shared_ptr<API::ITableWorkspace> FrameworkManagerProxy::retrieveTableWorkspace(const std::string& wsName)
{
  return boost::dynamic_pointer_cast<API::ITableWorkspace>(retrieveWorkspace(wsName));
//  API::ITableWorkspace_sptr table_wksp =  boost::dynamic_pointer_cast<API::ITableWorkspace>(retrieveWorkspace(wsName));
//  if( table_wksp.get() )
//  {
//    return table_wksp;
//  }
//  else
//  {
//    throw std::runtime_error("\"" + wsName + "\" is not a table workspace. "
//        "This may be a matrix workspace, try getMatrixWorkspace().");
//  }
}

/**
 * Return a pointer to a WorkspaceGroup
 * @param group_name :: The name of the group
 * @return A pointer to API::WorkspaceGroup object
 */
boost::shared_ptr<API::WorkspaceGroup> FrameworkManagerProxy::retrieveWorkspaceGroup(const std::string& group_name)
{
  return boost::dynamic_pointer_cast<API::WorkspaceGroup>(retrieveWorkspace(group_name));
//
//  API::WorkspaceGroup_sptr wksp_group = boost::dynamic_pointer_cast<API::WorkspaceGroup>(retrieveWorkspace(group_name));
//  if( wksp_group.get() )
//  {
//    return wksp_group;
//  }
//  else
//  {
//    throw std::runtime_error("\"" + group_name + "\" is not a group workspace. "
//        "This may be a matrix workspace, try getMatrixWorkspace().");
//  }
}

/**
 * Deletes a specified workspace.
 * @param wsName :: The name of the workspace to delete.
 * @return Boolean result.
 **/
bool FrameworkManagerProxy::deleteWorkspace(const std::string& wsName)
{
  return API::FrameworkManager::Instance().deleteWorkspace(wsName);
}

/**
 * Returns the name of all the workspaces.
 * @return Vector of strings.
 **/
std::set<std::string> FrameworkManagerProxy::getWorkspaceNames() const
{
  return API::AnalysisDataService::Instance().getObjectNames();
}

/**
 * Returns the names of all the workspace groups
 * @return A vector of strings.
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
 * @param group_name :: The name of the group
 * @return Vector of the names of the contained workspaces
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
  * Send an error log message to Mantid
  * @param msg :: The log message
  */
void FrameworkManagerProxy::sendErrorMessage(const std::string &msg)
{
    g_log.error(msg);
}

/**
  * Send a warning log message to Mantid
  * @param msg :: The log message
  */
void FrameworkManagerProxy::sendWarningMessage(const std::string &msg)
{
    g_log.warning(msg);
}

/**
 * Send a (notice) log message to Mantid
 * @param msg :: The log message
 */
void FrameworkManagerProxy::sendLogMessage(const std::string & msg) 
{
  g_log.notice(msg); 
}

/**
  * Send an information log message to Mantid
  * @param msg :: The log message
  */
void FrameworkManagerProxy::sendInformationMessage(const std::string &msg)
{
    g_log.information(msg);
}

/**
  * Send a debug log message to Mantid
  * @param msg :: The log message
  */
void FrameworkManagerProxy::sendDebugMessage(const std::string &msg)
{
    g_log.debug(msg);
}

/**
 * Check if a given workspace name exists in the ADS 
 * @param name :: A string specifying a name to check
 * @return true if it exists
 */
bool FrameworkManagerProxy::workspaceExists(const std::string & name) const
{
  return API::AnalysisDataService::Instance().doesExist(name);
}

/**
 * Add a python algorithm to the algorithm factory
 * @param pyobj :: The python algorithm object wrapped in a boost object
 */
void FrameworkManagerProxy::registerPyAlgorithm(boost::python::object pyobj)
{
  Mantid::API::CloneableAlgorithm* cppobj = boost::python::extract<Mantid::API::CloneableAlgorithm*>(pyobj);
  if( cppobj )
  {
    if( ! Mantid::API::AlgorithmFactory::Instance().storeCloneableAlgorithm(cppobj) )
    {
      g_log.error("Unable to register Python algorithm \"" + cppobj->name() + "\"");
    }
  }
  else
  {
    throw std::runtime_error("Unrecognized object type in Python algorithm registration.");
  }
}
  
//--------------------------------------------------------------------------
//
// Private member functions
//
//--------------------------------------------------------------------------
/**
 * Get a workspace pointer from the ADS
 * @param wsName :: The name of the workspace to retrieve, throws if the pointer is invalid
 * @return The workspace requested, a NotFoundError is thrown is it is not present
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
 * @param notice :: A pointer to a WorkspaceDeleteNotification object
 */
void FrameworkManagerProxy::
deleteNotificationReceived(Mantid::API::WorkspacePostDeleteNotification_ptr notice)
{
  /// This function may be overridden in Python
  workspaceRemoved(notice->object_name());  
}

/**
 * Utility function called when a workspace is added within the service
 * @param notice :: A pointer to a WorkspaceDeleteNotification object
 */
void FrameworkManagerProxy::
addNotificationReceived(Mantid::API::WorkspaceAddNotification_ptr notice)
{
  (void)notice;
}

/**
 * Utility function called when a workspace is replaced within the service
 * @param notice :: A pointer to a WorkspaceAfterReplaceNotification object
 */
void FrameworkManagerProxy::
replaceNotificationReceived(Mantid::API::WorkspaceAfterReplaceNotification_ptr notice)
{
  /// This function may be overridden in Python
  workspaceReplaced(notice->object_name());
}

/**
 * Utility function called when a workspace is replaced within the service
 * @param notice :: A pointer to a ClearADSNotification object
 */
void FrameworkManagerProxy::clearNotificationReceived(Mantid::API::ClearADSNotification_ptr)
{
  /// This function may be overridden in Python
  workspaceStoreCleared();
}

/**
 * Called by AlgorithmFactory updates
 * @param notice :: The nofification object
 */
void FrameworkManagerProxy::
handleAlgorithmFactoryUpdate(Mantid::API::AlgorithmFactoryUpdateNotification_ptr)
{
  //Call up to python via a virtual function
  algorithmFactoryUpdated();
}

void FrameworkManagerProxy::releaseFreeMemory()
{
  Mantid::API::MemoryManager::Instance().releaseFreeMemory();
}

/**
* Takes a string and removes the characters given in the optional second argument. If none are given then only alpha-numeric
* characters are retained.
* @param value The string to analyse
* @param cs A string of characters to remove
* @param eol_to_space Flag signalling whether end of line characters should be changed to a space
* @returns The sanitized value
*/
std::string FrameworkManagerProxy::removeCharacters(const std::string & value, const std::string & cs, bool eol_to_space)
{
  if (value.empty())
    return value;

  std::string retstring;
  std::string::const_iterator sIter = value.begin();
  std::string::const_iterator sEnd = value.end();

  // No characeters specified, only keep alpha-numeric
  if (cs.empty())
  {
    for (; sIter != sEnd; ++sIter)
    {
      int letter = static_cast<int> (*sIter);
      if ((letter >= 48 && letter <= 57) || (letter >= 97 && letter <= 122) || (letter >= 65 && letter <= 90) || (letter == 95))
      {
        retstring.push_back(*sIter);
      }
    }
  }
  else
  {
    for (; sIter != sEnd; ++sIter)
    {
      const char letter = (*sIter);
      // If the letter is NOT one to remove
      if (cs.find_first_of(letter) == std::string::npos)
      {
        //This is because I use single-quotes to delimit my strings in the module and if any in strings
        //that I try to write contain these, it will confuse Python so I'll convert them
        if (letter == '\'')
        {
          retstring.push_back('\"');
        }
        // Keep the character
        else
        {
          retstring.push_back(letter);
        }
      }
      else
      {
        if (eol_to_space && letter == '\n')
        {
          retstring.push_back(' ');
        }
      }
    }
  }
  return retstring;
}


}
}

