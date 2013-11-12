#ifndef MANTID_API_ALGORITHM_H_
#define MANTID_API_ALGORITHM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/PropertyManagerOwner.h"

// -- These headers will (most-likely) be used by every inheriting algorithm
#include "MantidAPI/Progress.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/EmptyValues.h"

#include <boost/shared_ptr.hpp>
#include <Poco/ActiveMethod.h>
#include <Poco/NotificationCenter.h>
#include <Poco/Notification.h>
#include <Poco/NObserver.h>
#include <Poco/Void.h>

#include <string>
#include <vector>
#include <map>
#include <cmath>

namespace Mantid
{
namespace API
{
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class AlgorithmProxy;
class AlgorithmHistory;

/**
 Base class from which all concrete algorithm classes should be derived.
 In order for a concrete algorithm class to do anything
 useful the methods init() & exec()  should be overridden.

 Further text from Gaudi file.......
 The base class provides utility methods for accessing
 standard services (event data service etc.); for declaring
 properties which may be configured by the job options
 service; and for creating Child Algorithms.
 The only base class functionality which may be used in the
 constructor of a concrete algorithm is the declaration of
 member variables as properties. All other functionality,
 i.e. the use of services and the creation of Child Algorithms,
 may be used only in initialise() and afterwards (see the
 Gaudi user guide).

 @author Russell Taylor, Tessella Support Services plc
 @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
 @date 12/09/2007

 Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://github.com/mantidproject/mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_API_DLL Algorithm : public IAlgorithm, public Kernel::PropertyManagerOwner
{
public:

  /// Base class for algorithm notifications
  class AlgorithmNotification: public Poco::Notification
  {
  public:
    AlgorithmNotification(const Algorithm* const alg):Poco::Notification(),m_algorithm(alg){}///< Constructor
    const IAlgorithm* algorithm() const {return m_algorithm;}                       ///< The algorithm
  private:
    const IAlgorithm* const m_algorithm;///< The algorithm
  };

  /// StartedNotification is sent when the algorithm begins execution.
  class StartedNotification: public AlgorithmNotification
  {
  public:
    StartedNotification(const Algorithm* const alg):AlgorithmNotification(alg){}///< Constructor
    virtual std::string name() const{return "StartedNotification";}///< class name
  };

  /// FinishedNotification is sent after the algorithm finishes its execution
  class FinishedNotification: public AlgorithmNotification
  {
  public:
    FinishedNotification(const Algorithm* const alg, bool res):AlgorithmNotification(alg),success(res){}///< Constructor
    virtual std::string name() const{return "FinishedNotification";}///< class name
    bool success;///< true if the finished algorithm was successful or false if it failed.
  };

  /// An algorithm can report its progress by sending ProgressNotification. Use
  /// Algorithm::progress(double) function to send a progress notification.
  class ProgressNotification: public AlgorithmNotification
  {
  public:
    /// Constructor
    ProgressNotification(const Algorithm* const alg, double p,const std::string& msg, double estimatedTime, int progressPrecision)
    : AlgorithmNotification(alg),progress(p),message(msg), estimatedTime(estimatedTime), progressPrecision(progressPrecision)
    { }
    virtual std::string name() const{return "ProgressNotification";}///< class name
    double progress;///< Current progress. Value must be between 0 and 1.
    std::string message;///< Message sent with notification
    double estimatedTime; ///<Estimated time to completion
    int progressPrecision; ///<Digits of precision to the progress (after the decimal).
  };

  /// ErrorNotification is sent when an exception is caught during execution of the algorithm.
  class ErrorNotification: public AlgorithmNotification
  {
  public:
    /// Constructor
    ErrorNotification(const Algorithm* const alg, const std::string& str):AlgorithmNotification(alg),what(str){}
    virtual std::string name() const{return "ErrorNotification";}///< class name
    std::string what;///< message string
  };

  /// CancelException is thrown to cancel execution of the algorithm. Use Algorithm::cancel() to
  /// terminate an algorithm. The execution will only be stopped if Algorithm::exec() method calls
  /// periodically Algorithm::interuption_point() which checks if Algorithm::cancel() has been called
  /// and throws CancelException if needed.
  class CancelException : public std::exception
  {
  public:
    CancelException():outMessage("Algorithm terminated"){}
    CancelException(const CancelException& A):outMessage(A.outMessage){}///< Copy constructor
    /// Assignment operator
    CancelException& operator=(const CancelException& A);
    /// Destructor
    ~CancelException() throw() {}

    /// Returns the message string.
    const char* what() const throw()
    {
      return outMessage.c_str();
    }
  private:
    /// The message returned by what()
    std::string outMessage;

  };


  //============================================================================
  Algorithm();
  virtual ~Algorithm();

  /** @name Algorithm Information */
  /// function to return a name of the algorithm, must be overridden in all algorithms
  virtual const std::string name() const = 0;
  /// function to return a version of the algorithm, must be overridden in all algorithms
  virtual int version() const = 0;
  /// function to return a category of the algorithm. A default implementation is provided
  virtual const std::string category() const {return "Misc";}
  /// Function to return all of the categories that contain this algorithm
  virtual const std::vector<std::string> categories() const;
  /// Function to return the sperator token for the category string. A default implementation ';' is provided
  virtual const std::string categorySeparator() const {return ";";}
  /// function to return any aliases to the algorithm;  A default implementation is provided
  virtual const std::string alias() const {return "";}

  const std::string workspaceMethodName() const;
  const std::vector<std::string> workspaceMethodOn() const;
  const std::string workspaceMethodInputProperty() const;

  /// Algorithm ID. Unmanaged algorithms return 0 (or NULL?) values. Managed ones have non-zero.
  AlgorithmID getAlgorithmID()const{return m_algorithmID;}

  /** @name IAlgorithm methods */
  void initialize();
  bool execute();
  void executeAsChildAlg();
  virtual std::map<std::string, std::string> validateInputs();
  virtual bool isInitialized() const;
  virtual bool isExecuted() const;
  bool isRunning() const;

  using Kernel::PropertyManagerOwner::getProperty;

  bool isChild() const;
  void setChild(const bool isChild);
  void enableHistoryRecordingForChild(const bool on);
  void setAlwaysStoreInADS(const bool doStore);
  void setRethrows(const bool rethrow);

  /** @name Asynchronous Execution */
  Poco::ActiveResult<bool> executeAsync();

  /// Add an observer for a notification
  void addObserver(const Poco::AbstractObserver& observer)const;

  /// Remove an observer
  void removeObserver(const Poco::AbstractObserver& observer)const;

  /// Raises the cancel flag.
  virtual void cancel();
  /// Returns the cancellation state
  bool getCancel() const { return m_cancel; }

  ///Logging can be disabled by passing a value of false
  void setLogging(const bool value){g_log.setEnabled(value);}
  ///returns the status of logging, True = enabled
  bool isLogging() const {return g_log.getEnabled();}

  ///sets the logging priority offset
  void setLoggingOffset(const int value) {g_log.setLevelOffset(value);}
  ///returns the logging priority offset
  int getLoggingOffset() const {return g_log.getLevelOffset();}

  /// Returns a reference to the logger.
  Kernel::Logger& getLogger() const { return g_log; }


  /// function returns an optional message that will be displayed in the default GUI, at the top.
  const std::string getOptionalMessage() const { return m_OptionalMessage;}

  /// Set an optional message that will be displayed in the default GUI, at the top.
  void setOptionalMessage(const std::string optionalMessage) { m_OptionalMessage = optionalMessage;}

  /// Get a summary to be used in the wiki page.
  const std::string getWikiSummary() const { return m_WikiSummary;}

  /// Set a summary to be used in the wiki page. Normally, this is approx. the same as the optional message.
  void setWikiSummary(const std::string WikiSummary) { m_WikiSummary = WikiSummary;}

  /// Get a description to be used in the wiki page.
  const std::string getWikiDescription() const { return m_WikiDescription;}

  /// Set a string to be used as the Description field in the wiki page.
  void setWikiDescription(const std::string WikiDescription) { m_WikiDescription = WikiDescription;}

  ///setting the child start progress
  void setChildStartProgress(const double startProgress)const{m_startChildProgress=startProgress;}
  /// setting the child end progress
  void setChildEndProgress(const double endProgress)const{m_endChildProgress=endProgress;}

  /** @name Serialization functions */
  //@{
  /// Serialize an object to a string
  virtual std::string toString() const;
  /// De-serialize an object from a string
  static IAlgorithm_sptr fromString(const std::string & input);
  /// Construct an object from a history entry
  static IAlgorithm_sptr fromHistory(const AlgorithmHistory & history);
  //@}

  boost::shared_ptr<Algorithm> createChildAlgorithm(const std::string& name, const double startProgress = -1.,
      const double endProgress = -1., const bool enableLogging=true, const int& version = -1);

protected:

  /// Virtual method - must be overridden by concrete algorithm
  virtual void init() = 0;
  /// Virtual method - must be overridden by concrete algorithm
  virtual void exec() = 0;
  /// Method defining summary, optional
  virtual void initDocs() {};

  /// Returns a semi-colon separated list of workspace types to attach this algorithm
  virtual const std::string workspaceMethodOnTypes() const { return ""; }

  void cacheWorkspaceProperties();

  friend class AlgorithmProxy;
  void initializeFromProxy(const AlgorithmProxy&);

  void setInitialized();
  void setExecuted(bool state);

  /// Sends notifications to observers. Observers can subscribe to notificationCenter
  /// using Poco::NotificationCenter::addObserver(...);
  mutable Poco::NotificationCenter m_notificationCenter;

  /** @name Progress Reporting functions */
  friend class Progress;
  void progress(double p, const std::string& msg = "", double estimatedTime = 0.0, int progressPrecision = 0);
  void interruption_point();

  ///Observation slot for child algorithm progress notification messages, these are scaled and then signalled for this algorithm.
  void handleChildProgressNotification(const Poco::AutoPtr<ProgressNotification>& pNf);
  ///Child algorithm progress observer
  Poco::NObserver<Algorithm, ProgressNotification> m_progressObserver;
  ///checks that the value was not set by users, uses the value in empty double/int.
  template <typename NumT>
  static bool isEmpty(const NumT toCheck);

  ///checks the property is a workspace property
  bool isWorkspaceProperty(const Kernel::Property* const prop) const;

  /// Set to true to stop execution
  bool m_cancel;
  /// Set if an exception is thrown, and not caught, within a parallel region
  bool m_parallelException;
  /// Reference to the logger class
  Kernel::Logger& g_log;

  friend class WorkspaceHistory; // Allow workspace history loading to adjust g_execCount
  static size_t g_execCount; ///< Counter to keep track of algorithm execution order

  // ------------------ For WorkspaceGroups ------------------------------------
  virtual bool checkGroups();
  virtual bool processGroups();
  virtual void setOtherProperties(IAlgorithm * alg, const std::string & propertyName, const std::string & propertyValue, int periodNum);
  typedef std::vector<boost::shared_ptr<Workspace> > WorkspaceVector;

  void findWorkspaceProperties(WorkspaceVector& inputWorkspaces,
      WorkspaceVector& outputWorkspaces) const;

  void copyNonWorkspaceProperties(IAlgorithm * alg, int periodNum);

  /// All the WorkspaceProperties that are Input or InOut. Set in execute()
  std::vector<IWorkspaceProperty *> m_inputWorkspaceProps;

private:
  /// Private Copy constructor: NO COPY ALLOWED
  Algorithm(const Algorithm&);
  /// Private assignment operator: NO ASSIGNMENT ALLOWED
  Algorithm& operator=(const Algorithm&);

  void lockWorkspaces();
  void unlockWorkspaces();

  void store();
  void fillHistory(Mantid::Kernel::DateAndTime, double,std::size_t);

  void logAlgorithmInfo() const;


  /// Poco::ActiveMethod used to implement asynchronous execution.
  Poco::ActiveMethod<bool, Poco::Void, Algorithm> m_executeAsync;
  bool executeAsyncImpl(const Poco::Void & i);

  // --------------------- Private Members -----------------------------------
  bool m_isInitialized; ///< Algorithm has been initialized flag
  bool m_isExecuted; ///< Algorithm is executed flag
  bool m_isChildAlgorithm; ///< Algorithm is a child algorithm
  bool m_recordHistoryForChild; ///< Flag to indicate whether history should be recorded. Applicable to child algs only
  bool m_alwaysStoreInADS; ///< Always store in the ADS, even for child algos
  bool m_runningAsync; ///< Algorithm is running asynchronously
  bool m_running; ///< Algorithm is running
  bool m_rethrow; ///< Algorithm should rethrow exceptions while executing
  mutable double m_startChildProgress; ///< Keeps value for algorithm's progress at start of an Child Algorithm
  mutable double m_endChildProgress; ///< Keeps value for algorithm's progress at Child Algorithm's finish
  AlgorithmID m_algorithmID; ///< Algorithm ID for managed algorithms
  std::string m_OptionalMessage; ///< An optional message string to be displayed in the GUI.
  std::string m_WikiSummary; ///< A summary line for the wiki page.
  std::string m_WikiDescription; ///< Description in the wiki page.
  std::vector<IAlgorithm_wptr> m_ChildAlgorithms; ///< A list of weak pointers to any child algorithms created


  /// Vector of all the workspaces that have been read-locked
  WorkspaceVector m_readLockedWorkspaces;
  /// Vector of all the workspaces that have been write-locked
  WorkspaceVector m_writeLockedWorkspaces;

  /// All the WorkspaceProperties that are Output or InOut. Set in execute()
  std::vector<IWorkspaceProperty *> m_outputWorkspaceProps;
  /// All the WorkspaceProperties that are Output (not inOut). Set in execute()
  std::vector<IWorkspaceProperty *> m_pureOutputWorkspaceProps;

  /// One vector of workspaces for each input workspace property
  std::vector<WorkspaceVector> m_groups;
  /// Pointer to the WorkspaceGroup (if any) for each input workspace property
  std::vector<boost::shared_ptr<WorkspaceGroup> > m_groupWorkspaces;
  /// If only one input is a group, this is its index. -1 if they are all groups
  int m_singleGroup;
  /// Size of the group(s) being processed
  size_t m_groupSize;
  /// All the groups have similar names (group_1, group_2 etc.)
  bool m_groupsHaveSimilarNames;
  /// A non-recursive mutex for thread-safety
  mutable Kernel::Mutex m_mutex;
};

///Typedef for a shared pointer to an Algorithm
typedef boost::shared_ptr<Algorithm> Algorithm_sptr;

} // namespace API
} // namespace Mantid

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */
#define DECLARE_ALGORITHM(classname) \
    namespace { \
  Mantid::Kernel::RegistrationHelper register_alg_##classname( \
      ((Mantid::API::AlgorithmFactory::Instance().subscribe<classname>()) \
          , 0)); \
}

#endif /*MANTID_API_ALGORITHM_H_*/
