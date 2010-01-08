#ifndef MANTID_PYTHONAPI_FRAMEWORKMANAGER_H_
#define MANTID_PYTHONAPI_FRAMEWORKMANAGER_H_

//----------------------------------
// Includes
//----------------------------------
#include <string>
#include <vector>
#include <Python.h>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include <MantidAPI/AnalysisDataService.h>

#include <Poco/NObserver.h>


//-------------------------------
// Mantid forward declarations
//--------------------------------
namespace Mantid
{

namespace API
{
  class IAlgorithm;
  class Algorithm;
  class MatrixWorkspace;
  class ITableWorkspace;
  class WorkspaceGroup;
}

namespace PythonAPI
{

/** 
    FrameworkManager is a wrapper for the API::FrameworkManager class in Mantid. 
    As FrameworkManager is a singleton in Mantid it was easier to create a wrapper
    class to be used from python.

    @author ISIS, STFC
    @date 28/02/2008

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/
class DLLExport FrameworkManagerProxy
{
public:
  /// Constructor
  FrameworkManagerProxy();
  /// Destructor
  virtual ~FrameworkManagerProxy();

  /** @name Memory functions */
  //@{
  /// Clears all memory associated with the FrameworkManager 
  void clear();
  /// Clear memory associated with the AlgorithmManager
  void clearAlgorithms();
  /// Clear memory associated with the ADS
  void clearData();
  /// Clear memory associated with the ADS
  void clearInstruments();	
  //@}

  /** @name Config access */
  //@{
  /// Access a property from the .properties file
  std::string getConfigProperty(const std::string & key) const;
  //@}

  /** @name Algorithm control */
  //@{
  /// Creates and instance of an algorithm
  API::IAlgorithm* createAlgorithm(const std::string& algName);
  /// Creates and instance of an algorithm of a specific version
  API::IAlgorithm* createAlgorithm(const std::string& algName, const int& version);
  // Creates an instance of an algorithm and sets the properties provided
  API::IAlgorithm* createAlgorithm(const std::string& algName, const std::string& propertiesArray);
  // Creates an instance of an algorithm of a specific version and sets the properties provided
  API::IAlgorithm* createAlgorithm(const std::string& algName, const std::string& propertiesArray,const int& version);
  /// Creates an instance of an algorithm, sets the properties provided & then executes it.
  API::IAlgorithm* execute(const std::string& algName, const std::string& propertiesArray);
  /// Creates an algorithm of a given version, sets the properties provided & then executes it.
  API::IAlgorithm* execute(const std::string& algName, const std::string& propertiesArray,const int& version);
  //@}
  
  /** @name Workspace related functions */ 
  //@{
  /// Returns a pointer to the MatrixWorkspace requested
  boost::shared_ptr<API::MatrixWorkspace> retrieveMatrixWorkspace(const std::string& wsName);
  /// Returns a pointer to the TableWorkspace requested
  boost::shared_ptr<API::ITableWorkspace> retrieveTableWorkspace(const std::string& wsName);
  /// Returns a list of pointers to the MatrixWorkspace objects with a group
  boost::shared_ptr<API::WorkspaceGroup> retrieveWorkspaceGroup(const std::string& group_name);
  /// Deletes a workspace from the framework
  bool deleteWorkspace(const std::string& wsName);
  /// Return the list of currently available workspace names
  std::set<std::string> getWorkspaceNames() const;
  /// Return a list of the currently available workspace groups
  std::set<std::string> getWorkspaceGroupNames() const;
  /// Return the list of names within a workspace group
  std::vector<std::string> getWorkspaceGroupEntries(const std::string & group_name) const;
  /// Check if a given workspace exists in the ADS
  bool workspaceExists(const std::string & name) const;
  /** 
   * A function that can be overridden in Python to handle the removal of a workspace from the ADS
   * @param name The name of the workspace
   */
  virtual void workspaceRemoved(const std::string & name) {}
  /** 
   * A function that can be overridden in Python to handle the adding a workspace to the ADS
   * @param name The name of the workspace
   */
  virtual void workspaceAdded(const std::string & name) {}
  /** 
   * A function that can be overridden in Python to handle the replacing a workspace to the ADS
   * @param name The name of the workspace
   */
  virtual void workspaceReplaced(const std::string & name) {}
  /**
   * A function that notifies Python that that the workspace store has been cleared
   */
  virtual void workspaceStoreCleared() {}
  //@}

  /// Set GIL state
  void setGILRequired(bool state) { m_gil_required = state; }

  /// Return the list of currently registered algorithm names
  std::vector<std::string> getAlgorithmNames() const;
  //Send a log message to the Mantid Framework with a specified priority
  void sendLogMessage(const std::string & msg);
  /// Create the simple Python API for Mantid
  void createPythonSimpleAPI(bool);

  /** Python algorithms */
  //@{
  /// Add a Python alogirthm
  int addPythonAlgorithm(PyObject* pyAlg);
  /// Execute a Python algorithm
  void executePythonAlgorithm(std::string algName);
  //@}

 protected:
  /** A flag indicating whether code has been executed from within Python. This is a bit of a hack
   * so that the notification signals don't cause deadlocks if they happen to get called from
   * a thread that did not originate from within Python
   */
  static bool m_gil_required;

 private:
  /// Copy constructor
  FrameworkManagerProxy(const FrameworkManagerProxy&);
  /// Assignment operator
  FrameworkManagerProxy& operator =(const FrameworkManagerProxy&);

  /// Get a workspace pointer from the ADS
  boost::shared_ptr<Mantid::API::Workspace> retrieveWorkspace(const std::string & wsName);
  /// Function called when observer objects recieves a notification
  void deleteNotificationReceived(Mantid::API::WorkspaceDeleteNotification_ptr notice);
  /// Poco delete notification observer object
  Poco::NObserver<FrameworkManagerProxy, Mantid::API::WorkspaceDeleteNotification> m_delete_observer;
  /// Function called when observer objects recieve an add notification
  void addNotificationReceived(Mantid::API::WorkspaceAddNotification_ptr notice);
  /// Poco add notification observer object
  Poco::NObserver<FrameworkManagerProxy, Mantid::API::WorkspaceAddNotification> m_add_observer;
  /// Function called when observer objects recieves a replace notification
  void replaceNotificationReceived(Mantid::API::WorkspaceAfterReplaceNotification_ptr notice);
  /// Poco delete notification observer object
  Poco::NObserver<FrameworkManagerProxy, Mantid::API::WorkspaceAfterReplaceNotification> m_replace_observer;
  /// Function called when observer objects recieves a replace notification
  void clearNotificationReceived(Mantid::API::ClearADSNotification_ptr notice);
  /// Poco clear notification observer object
  Poco::NObserver<FrameworkManagerProxy, Mantid::API::ClearADSNotification> m_clear_observer;

  /// A Python logger
  static Mantid::Kernel::Logger& g_log;
 
};

}
}

#endif //MANTID_PYTHONAPI_FRAMEWORKMANAGER_H_
