#ifndef MANTID_API_WORKSPACETRACER_H_
#define MANTID_API_WORKSPACETRACER_H_

//-----------------------------------
//Includes
//-----------------------------------
#include "MantidAPI/DllExport.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/Algorithm.h"

#include "Poco/NObserver.h"
#include "Poco/ActiveMethod.h"
#include "Poco/Void.h"

#include <deque>
#include <string>

//-----------------------------------
// Forward declarations
//-----------------------------------
namespace Mantid
{

namespace Kernel
{
  class Logger;
  class PropertyHistory;
}

namespace API
{

  class AlgorithmHistory;

/** 
    This class is designed to trace out the algorithms that were performed on a given workspace,
    with the aim of being able to automatically rerun a chain once a specified workspace has been
    refreshed.

    @author Martyn Gigg, Tessella Support Services plc
    @date 04/12/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class EXPORT_OPT_MANTID_API WorkspaceTracerImpl
{
  
 private:
  /**
   *  A nested class to store an algorithm chain and other properties
   *  associated with it
   */
  class AlgorithmChain
  {
    
  public:

    /**
     * Default constructor
     */
    AlgorithmChain() : m_Algorithms() {}

    /**
     * Insert an algorithm at the end of the chain
     * @param alg The algorithm pointer to insert
     */
    void addToEnd(Mantid::API::Algorithm* alg) 
    { 
      m_Algorithms.push_back(alg);
    }

    /**
     * Is the list empty
     * @returns A boolean indicating if the list is empty
     */
    bool isEmpty() const { return m_Algorithms.empty(); }

    // /Execute the algorithm chain
    void executeChain();
        
  private:
    /// A list of algorithms to rerun
    std::deque<Mantid::API::Algorithm*> m_Algorithms;
  };


public:
  /**
   * Get the workspace name that is being traced
   * @returns A string indicating the workspace that has been set to analyse
   */
  const std::string & getBaseWorkspace() const { return m_strWsName; }
  
  ///Notification handler
  void handleAfterReplaceNotification(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf);

  ///Notification observer
  Poco::NObserver<WorkspaceTracerImpl, Mantid::API::WorkspaceAfterReplaceNotification> m_wkspAftReplaceObserver;

private:
  /// Specialisation of the CreateUsingNew structure for this class. Needs to be a friend to access the private constructor
  friend struct Mantid::Kernel::CreateUsingNew<WorkspaceTracerImpl>;

  ///Method to start the trace in a separate thread
  Poco::ActiveMethod<Poco::Void, std::string, WorkspaceTracerImpl> executeTrace;
  ///The helper member function for the separate thread execution
  Poco::Void executeTraceImpl(const std::string & arg);
 
  /// Private Constructor
  WorkspaceTracerImpl();

  /// Private Destructor
  virtual ~WorkspaceTracerImpl();

  /// No copy allowed
  WorkspaceTracerImpl(const WorkspaceTracerImpl&);

  /// No assignment allowed
  WorkspaceTracerImpl& operator=(const WorkspaceTracerImpl&);

private:

  //---------------------
  // Private utility functions
  //---------------------
  ///Find the list of algorithms to rerun given that the one specifed as an argument has been refreshed
  void createAlgorithmList();
  
  /// Does the property exists in the workspace history
  bool propertyExists(const std::string & wsName, const std::string & pvalue, unsigned int dir);

  /// Does the property exists in the algorithm history
  bool propertyExists(const AlgorithmHistory & algHist, const std::string & pvalue, unsigned int dir);

  /// Get the algorithm history for the refreshed workspace but taking care of things like algorithms with the same input/output workspace  
  void getBaseAlgorithmChain(std::vector<Mantid::API::AlgorithmHistory> & baseChain);

  /// Check it algorithm history takes a workspace as input
  bool hasWorkspaceInput(const Mantid::API::AlgorithmHistory & algHist);

  /// Get or create an algorithm pointer based on the given AlgorithmHistory object
  Mantid::API::Algorithm* getAlgorithm(const AlgorithmHistory & algHist);

  /// Is the property history related to a workspace
  bool isWorkspaceProperty(const Mantid::Kernel::PropertyHistory & prop);

  /// Remake a workspace that has been deleted
  Algorithm* remakeWorkspace(const std::string & wsName);

 private:

  //---------------------
  // Member variables
  //---------------------
  /// The name of the workspace that was replaced
  std::string m_strWsName;
  /// The history of algorithms performed on the replaced workspace
  std::vector<AlgorithmHistory> m_vecAlgHistories;
  /// Stores the chain of algorithms to run and handles popping off things that have finished
  AlgorithmChain m_algChain;

  /// A mutex to ensure serialised data access
  Poco::Mutex m_mutex;
  /// Is the chain currently running
  bool m_isRunning;
  /// Should we be switched on, based upon the parameter in the Mantid.properties config file
  bool m_isSwitchedOn;
  
  ///A reference to the logger
  static Kernel::Logger& g_log;
};

  ///Forward declaration of a specialisation of SingletonHolder for 
  ///WorkspaceTracerImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
  // this breaks new namespace declaraion rules; need to find a better fix
  template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<WorkspaceTracerImpl>;
#endif /* _WIN32 */
  
  /// The specialisation of the SingletonHolder for this class
  typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<WorkspaceTracerImpl> WorkspaceTracer;

} // namepace API

} // namespace Mantid

#endif //MANTID_API_WORKSPACETRACER_H_
