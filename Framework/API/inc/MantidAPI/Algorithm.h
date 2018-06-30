#ifndef MANTID_API_ALGORITHM_H_
#define MANTID_API_ALGORITHM_H_

#include <atomic>

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IndexTypeProperty.h"
#include "MantidKernel/PropertyManagerOwner.h"

// -- These headers will (most-likely) be used by every inheriting algorithm
#include "MantidAPI/AlgorithmFactory.h" //for the factory macro
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/MultiThreaded.h"

#include "MantidParallel/ExecutionMode.h"
#include "MantidParallel/StorageMode.h"
namespace boost {
template <class T> class weak_ptr;
}

namespace Poco {
template <class R, class A, class O, class S> class ActiveMethod;
template <class O> class ActiveStarter;
class NotificationCenter;
template <class C, class N> class NObserver;
class Void;
} // namespace Poco

namespace Json {
class Value;
}

namespace Mantid {
namespace Indexing {
class SpectrumIndexSet;
}
namespace Parallel {
class Communicator;
}
namespace API {
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
@author Based on the Gaudi class of the same name (see
http://proj-gaudi.web.cern.ch/proj-gaudi/)
@date 12/09/2007

Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class MANTID_API_DLL Algorithm : public IAlgorithm,
                                 public Kernel::PropertyManagerOwner {
public:
  /// Base class for algorithm notifications
  class MANTID_API_DLL AlgorithmNotification : public Poco::Notification {
  public:
    AlgorithmNotification(const Algorithm *const alg);
    const IAlgorithm *algorithm() const;

  private:
    const IAlgorithm *const m_algorithm; ///< The algorithm
  };

  /// StartedNotification is sent when the algorithm begins execution.
  class MANTID_API_DLL StartedNotification : public AlgorithmNotification {
  public:
    StartedNotification(const Algorithm *const alg);
    std::string name() const override;
  };

  /// FinishedNotification is sent after the algorithm finishes its execution
  class MANTID_API_DLL FinishedNotification : public AlgorithmNotification {
  public:
    FinishedNotification(const Algorithm *const alg, bool res);
    std::string name() const override;
    bool success; ///< true if the finished algorithm was successful or false if
                  /// it failed.
  };

  /// An algorithm can report its progress by sending ProgressNotification. Use
  /// Algorithm::progress(double) function to send a progress notification.
  class MANTID_API_DLL ProgressNotification : public AlgorithmNotification {
  public:
    /// Constructor
    ProgressNotification(const Algorithm *const alg, double p,
                         const std::string &msg, double estimatedTime,
                         int progressPrecision);
    std::string name() const override;
    double progress;       ///< Current progress. Value must be between 0 and 1.
    std::string message;   ///< Message sent with notification
    double estimatedTime;  ///< Estimated time to completion
    int progressPrecision; ///< Digits of precision to the progress (after the
                           /// decimal).
  };

  /// ErrorNotification is sent when an exception is caught during execution of
  /// the algorithm.
  class MANTID_API_DLL ErrorNotification : public AlgorithmNotification {
  public:
    /// Constructor
    ErrorNotification(const Algorithm *const alg, const std::string &str);
    std::string name() const override;
    std::string what; ///< message string
  };

  /// CancelException is thrown to cancel execution of the algorithm. Use
  /// Algorithm::cancel() to
  /// terminate an algorithm. The execution will only be stopped if
  /// Algorithm::exec() method calls
  /// periodically Algorithm::interuption_point() which checks if
  /// Algorithm::cancel() has been called
  /// and throws CancelException if needed.
  class MANTID_API_DLL CancelException : public std::exception {
  public:
    /// Returns the message string.
    const char *what() const noexcept override;
  };

  //============================================================================
  Algorithm();
  Algorithm(const Algorithm &) = delete;
  Algorithm &operator=(const Algorithm &) = delete;
  ~Algorithm() override;

  /** @name Algorithm Information */
  /// function to return a name of the algorithm, must be overridden in all
  /// algorithms
  const std::string name() const override = 0;
  /// function to return a version of the algorithm, must be overridden in all
  /// algorithms
  int version() const override = 0;
  /// function returns a summary message that will be displayed in the default
  /// GUI, and in the help.
  const std::string summary() const override = 0;
  /// function to return a category of the algorithm. A default implementation
  /// is provided
  const std::string category() const override { return "Misc"; }
  /// Function to return all of the categories that contain this algorithm
  const std::vector<std::string> categories() const override;
  /// Function to return the separator token for the category string. A default
  /// implementation ';' is provided
  const std::string categorySeparator() const override { return ";"; }
  /// Function to return all of the seeAlso (these are not validated) algorithms
  /// related to this algorithm.A default implementation is provided.
  const std::vector<std::string> seeAlso() const override { return {}; };
  /// function to return any aliases to the algorithm;  A default implementation
  /// is provided
  const std::string alias() const override { return ""; }

  /// function to return URL for algorithm documentation; A default
  /// implementation is provided.
  /// Override if the algorithm is not part of the Mantid distribution.
  const std::string helpURL() const override { return ""; }

  template <typename T, typename = typename std::enable_if<std::is_convertible<
                            T *, MatrixWorkspace *>::value>::type>
  std::tuple<boost::shared_ptr<T>, Indexing::SpectrumIndexSet>
  getWorkspaceAndIndices(const std::string &name) const;

  template <typename T1, typename T2,
            typename = typename std::enable_if<
                std::is_convertible<T1 *, MatrixWorkspace *>::value>::type,
            typename = typename std::enable_if<
                std::is_convertible<T2 *, std::string *>::value ||
                std::is_convertible<T2 *, std::vector<int64_t> *>::value>::type>
  void setWorkspaceInputProperties(const std::string &name,
                                   const boost::shared_ptr<T1> &wksp,
                                   IndexType type, const T2 &list);

  template <typename T1, typename T2,
            typename = typename std::enable_if<
                std::is_convertible<T1 *, MatrixWorkspace *>::value>::type,
            typename = typename std::enable_if<
                std::is_convertible<T2 *, std::string *>::value ||
                std::is_convertible<T2 *, std::vector<int64_t> *>::value>::type>
  void setWorkspaceInputProperties(const std::string &name,
                                   const std::string &wsName, IndexType type,
                                   const T2 &list);

  const std::string workspaceMethodName() const override;
  const std::vector<std::string> workspaceMethodOn() const override;
  const std::string workspaceMethodInputProperty() const override;

  /// Algorithm ID. Unmanaged algorithms return 0 (or NULL?) values. Managed
  /// ones have non-zero.
  AlgorithmID getAlgorithmID() const override { return m_algorithmID; }

  /** @name IAlgorithm methods */
  void initialize() override;
  bool execute() override;
  void executeAsChildAlg() override;
  std::map<std::string, std::string> validateInputs() override;
  bool isInitialized() const override;
  bool isExecuted() const override;
  bool isRunning() const override;

  using Kernel::PropertyManagerOwner::getProperty;

  bool isChild() const override;
  void setChild(const bool isChild) override;
  void enableHistoryRecordingForChild(const bool on) override;
  bool isRecordingHistoryForChild() { return m_recordHistoryForChild; }
  void setAlwaysStoreInADS(const bool doStore) override;
  bool getAlwaysStoreInADS() const override;
  void setRethrows(const bool rethrow) override;

  /** @name Asynchronous Execution */
  Poco::ActiveResult<bool> executeAsync() override;

  /// Add an observer for a notification
  void addObserver(const Poco::AbstractObserver &observer) const override;

  /// Remove an observer
  void removeObserver(const Poco::AbstractObserver &observer) const override;

  /// Raises the cancel flag.
  void cancel() override;
  bool getCancel() const;

  Kernel::Logger &getLogger() const;
  void setLogging(const bool value) override;
  bool isLogging() const override;
  void setLoggingOffset(const int value) override;
  int getLoggingOffset() const override;
  /// disable Logging of start and end messages
  void setAlgStartupLogging(const bool enabled) override;
  /// get the state of Logging of start and end messages
  bool getAlgStartupLogging() const override;

  /// setting the child start progress
  void setChildStartProgress(const double startProgress) const override {
    m_startChildProgress = startProgress;
  }
  /// setting the child end progress
  void setChildEndProgress(const double endProgress) const override {
    m_endChildProgress = endProgress;
  }

  /** @name Serialization functions */
  //@{
  /// Serialize an object to a string
  std::string toString() const override;
  /// Serialize an object to a json object
  ::Json::Value toJson() const;
  /// De-serialize an object from a string
  static IAlgorithm_sptr fromString(const std::string &input);
  /// Construct an object from a history entry
  static IAlgorithm_sptr fromHistory(const AlgorithmHistory &history);
  //@}

  virtual boost::shared_ptr<Algorithm> createChildAlgorithm(
      const std::string &name, const double startProgress = -1.,
      const double endProgress = -1., const bool enableLogging = true,
      const int &version = -1);
  void setupAsChildAlgorithm(boost::shared_ptr<Algorithm> algorithm,
                             const double startProgress = -1.,
                             const double endProgress = -1.,
                             const bool enableLogging = true);

  /// set whether we wish to track the child algorithm's history and pass it the
  /// parent object to fill.
  void trackAlgorithmHistory(boost::shared_ptr<AlgorithmHistory> parentHist);

  using WorkspaceVector = std::vector<boost::shared_ptr<Workspace>>;

  void findWorkspaceProperties(WorkspaceVector &inputWorkspaces,
                               WorkspaceVector &outputWorkspaces) const;

  // ------------------ For WorkspaceGroups ------------------------------------
  virtual bool checkGroups();

  virtual bool processGroups();

  void copyNonWorkspaceProperties(IAlgorithm *alg, int periodNum);

  const Parallel::Communicator &communicator() const;
  void setCommunicator(const Parallel::Communicator &communicator);

protected:
  /// Virtual method - must be overridden by concrete algorithm
  virtual void init() = 0;
  /// Virtual method - must be overridden by concrete algorithm
  virtual void exec() = 0;

  void exec(Parallel::ExecutionMode executionMode);
  virtual void execDistributed();
  virtual void execMasterOnly();

  virtual Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes) const;

  /// Returns a semi-colon separated list of workspace types to attach this
  /// algorithm
  virtual const std::string workspaceMethodOnTypes() const { return ""; }

  void cacheWorkspaceProperties();

  friend class AlgorithmProxy;
  void initializeFromProxy(const AlgorithmProxy &);

  void setInitialized();
  void setExecuted(bool state);

  void store();

  /** @name Progress Reporting functions */
  friend class Progress;
  void progress(double p, const std::string &msg = "",
                double estimatedTime = 0.0, int progressPrecision = 0);
  void interruption_point();

  /// Return a reference to the algorithm's notification dispatcher
  Poco::NotificationCenter &notificationCenter() const;

  /// Observation slot for child algorithm progress notification messages, these
  /// are scaled and then signalled for this algorithm.
  void handleChildProgressNotification(
      const Poco::AutoPtr<ProgressNotification> &pNf);
  /// Return a reference to the algorithm's object that is reporting progress
  const Poco::AbstractObserver &progressObserver() const;

  /// checks that the value was not set by users, uses the value in empty
  /// double/int.
  template <typename NumT> static bool isEmpty(const NumT toCheck);

  /// checks the property is a workspace property
  bool isWorkspaceProperty(const Kernel::Property *const prop) const;

  /// get whether we are tracking the history for this algorithm,
  bool trackingHistory();
  /// Copy workspace history for input workspaces to output workspaces and
  /// record the history for ths algorithm
  virtual void fillHistory();

  /// Set to true to stop execution
  std::atomic<bool> m_cancel;
  /// Set if an exception is thrown, and not caught, within a parallel region
  std::atomic<bool> m_parallelException;

  friend class WorkspaceHistory; // Allow workspace history loading to adjust
                                 // g_execCount
  static size_t
      g_execCount; ///< Counter to keep track of algorithm execution order

  virtual void setOtherProperties(IAlgorithm *alg,
                                  const std::string &propertyName,
                                  const std::string &propertyValue,
                                  int periodNum);

  /// All the WorkspaceProperties that are Input or InOut. Set in execute()
  std::vector<IWorkspaceProperty *> m_inputWorkspaceProps;
  /// Pointer to the history for the algorithm being executed
  boost::shared_ptr<AlgorithmHistory> m_history;

  /// Logger for this algorithm
  Kernel::Logger m_log;
  Kernel::Logger &g_log;

  /// Pointer to the parent history object (if set)
  boost::shared_ptr<AlgorithmHistory> m_parentHistory;

  /// One vector of workspaces for each input workspace property
  std::vector<WorkspaceVector> m_groups;
  /// Size of the group(s) being processed
  size_t m_groupSize;
  /// distinguish between base processGroups() and overriden/algorithm specific
  /// versions
  bool m_usingBaseProcessGroups = false;

  template <typename T, const int AllowedIndexTypes = IndexType::WorkspaceIndex,
            typename... WSPropArgs,
            typename = typename std::enable_if<
                std::is_convertible<T *, MatrixWorkspace *>::value>::type>
  void declareWorkspaceInputProperties(const std::string &propertyName,
                                       const std::string &doc,
                                       WSPropArgs &&... wsPropArgs);

private:
  template <typename T1, typename T2, typename WsType>
  void doSetInputProperties(const std::string &name, const T1 &wksp,
                            IndexType type, const T2 &list);
  void lockWorkspaces();
  void unlockWorkspaces();

  void linkHistoryWithLastChild();

  void logAlgorithmInfo() const;

  bool executeAsyncImpl(const Poco::Void &i);

  bool doCallProcessGroups(Mantid::Types::Core::DateAndTime &start_time);

  // Report that the algorithm has completed.
  void reportCompleted(const double &duration,
                       const bool groupProcessing = false);

  void registerFeatureUsage() const;

  Parallel::ExecutionMode getExecutionMode() const;
  std::map<std::string, Parallel::StorageMode>
  getInputWorkspaceStorageModes() const;
  void setupSkipValidationMasterOnly();

  bool isCompoundProperty(const std::string &name) const;

  // --------------------- Private Members -----------------------------------
  /// Poco::ActiveMethod used to implement asynchronous execution.
  Poco::ActiveMethod<bool, Poco::Void, Algorithm,
                     Poco::ActiveStarter<Algorithm>> *m_executeAsync;

  /// Sends notifications to observers. Observers can subscribe to
  /// notificationCenter
  /// using Poco::NotificationCenter::addObserver(...);
  mutable Poco::NotificationCenter *m_notificationCenter;
  /// Child algorithm progress observer
  mutable Poco::NObserver<Algorithm, ProgressNotification> *m_progressObserver;

  bool m_isInitialized;         ///< Algorithm has been initialized flag
  bool m_isExecuted;            ///< Algorithm is executed flag
  bool m_isChildAlgorithm;      ///< Algorithm is a child algorithm
  bool m_recordHistoryForChild; ///< Flag to indicate whether history should be
                                /// recorded. Applicable to child algs only
  bool m_alwaysStoreInADS; ///< Always store in the ADS, even for child algos
  bool m_runningAsync;     ///< Algorithm is running asynchronously
  std::atomic<bool> m_running; ///< Algorithm is running
  bool m_rethrow; ///< Algorithm should rethrow exceptions while executing
  bool m_isAlgStartupLoggingEnabled; /// Whether to log alg startup and
                                     /// closedown messages from the base class
                                     /// (default = true)
  mutable double m_startChildProgress; ///< Keeps value for algorithm's progress
                                       /// at start of an Child Algorithm
  mutable double m_endChildProgress;   ///< Keeps value for algorithm's progress
                                       /// at Child Algorithm's finish
  AlgorithmID m_algorithmID;           ///< Algorithm ID for managed algorithms
  std::vector<boost::weak_ptr<IAlgorithm>> m_ChildAlgorithms; ///< A list of
                                                              /// weak pointers
                                                              /// to any child
                                                              /// algorithms
                                                              /// created

  /// Vector of all the workspaces that have been read-locked
  WorkspaceVector m_readLockedWorkspaces;
  /// Vector of all the workspaces that have been write-locked
  WorkspaceVector m_writeLockedWorkspaces;

  /// All the WorkspaceProperties that are Output or InOut. Set in execute()
  std::vector<IWorkspaceProperty *> m_outputWorkspaceProps;
  /// All the WorkspaceProperties that are Output (not inOut). Set in execute()
  std::vector<IWorkspaceProperty *> m_pureOutputWorkspaceProps;

  /// Pointer to the WorkspaceGroup (if any) for each input workspace property
  std::vector<boost::shared_ptr<WorkspaceGroup>> m_groupWorkspaces;
  /// If only one input is a group, this is its index. -1 if they are all groups
  int m_singleGroup;
  /// All the groups have similar names (group_1, group_2 etc.)
  bool m_groupsHaveSimilarNames;

  std::vector<std::string> m_reservedList;

  /// (MPI) communicator used when executing the algorithm.
  std::unique_ptr<Parallel::Communicator> m_communicator;
};

/// Typedef for a shared pointer to an Algorithm
using Algorithm_sptr = boost::shared_ptr<Algorithm>;

} // namespace API
} // namespace Mantid

/* Used to register classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 */
#define DECLARE_ALGORITHM(classname)                                           \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_alg_##classname((                \
      (Mantid::API::AlgorithmFactory::Instance().subscribe<classname>()), 0)); \
  }

#endif /*MANTID_API_ALGORITHM_H_*/
