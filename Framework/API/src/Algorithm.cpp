// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/DeprecatedAlias.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/WorkspacePropertyUtils.h"

#include "MantidJson/Json.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UsageService.h"

#include "MantidKernel/StringTokenizer.h"
#include <Poco/ActiveMethod.h>
#include <Poco/ActiveResult.h>
#include <Poco/NotificationCenter.h>
#include <Poco/RWLock.h>
#include <Poco/Void.h>

#include <H5Cpp.h>
#include <json/json.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <utility>

// Index property handling template definitions
#include "MantidAPI/Algorithm.tcc"

using namespace Mantid::Kernel;

namespace Mantid {
namespace API {
namespace {
/// Separator for workspace types in workspaceMethodOnTypes member
const std::string WORKSPACE_TYPES_SEPARATOR = ";";

/// The minimum number of seconds after execution that the algorithm should be
/// kept alive before garbage collection
const size_t DELAY_BEFORE_GC = 5;

class WorkspacePropertyValueIs {
public:
  explicit WorkspacePropertyValueIs(const std::string &value) : m_value(value) {}
  bool operator()(IWorkspaceProperty *property) {
    auto *prop = dynamic_cast<Property *>(property);
    if (!prop)
      return false;
    return prop->value() == m_value;
  }

private:
  const std::string &m_value;
};

template <typename T> struct RunOnFinish {
  RunOnFinish(T &&task) : m_onfinsh(std::move(task)) {}
  ~RunOnFinish() { m_onfinsh(); }

private:
  T m_onfinsh;
};

} // namespace

// Doxygen can't handle member specialization at the moment:
// https://bugzilla.gnome.org/show_bug.cgi?id=406027
// so we have to ignore them
///@cond
template <typename NumT> bool Algorithm::isEmpty(const NumT toCheck) {
  return static_cast<int>(toCheck) == EMPTY_INT();
}

template <> MANTID_API_DLL bool Algorithm::isEmpty(const double toCheck) {
  return std::abs((toCheck - EMPTY_DBL()) / (EMPTY_DBL())) < 1e-8;
}

// concrete instantiations
template MANTID_API_DLL bool Algorithm::isEmpty<int>(const int);
template MANTID_API_DLL bool Algorithm::isEmpty<int64_t>(const int64_t);
template MANTID_API_DLL bool Algorithm::isEmpty<std::size_t>(const std::size_t);
///@endcond

//=============================================================================================
//================================== Constructors/Destructors
//=================================
//=============================================================================================

/// Initialize static algorithm counter
size_t Algorithm::g_execCount = 0;

/// Constructor
Algorithm::Algorithm()
    : m_cancel(false), m_parallelException(false), m_log("Algorithm"), g_log(m_log), m_groupSize(0),
      m_executeAsync(nullptr), m_notificationCenter(nullptr), m_progressObserver(nullptr),
      m_executionState(ExecutionState::Uninitialized), m_resultState(ResultState::NotFinished),
      m_isChildAlgorithm(false), m_recordHistoryForChild(false), m_alwaysStoreInADS(true), m_runningAsync(false),
      m_rethrow(false), m_isAlgStartupLoggingEnabled(true), m_startChildProgress(0.), m_endChildProgress(0.),
      m_algorithmID(this), m_singleGroup(-1), m_groupsHaveSimilarNames(false), m_inputWorkspaceHistories(),
      m_properties() {}

/// Virtual destructor
Algorithm::~Algorithm() = default;

//=============================================================================================
//================================== Simple Getters/Setters
//===================================
//=============================================================================================

/// Gets the current execution state
ExecutionState Algorithm::executionState() const { return m_executionState; }

/// Sets the current execution state
void Algorithm::setExecutionState(const ExecutionState state) { m_executionState = state; }

/// Gets the current result State
ResultState Algorithm::resultState() const { return m_resultState; }

/// Sets the result execution state
/// if set to Success or Failed will also set the execution state to finished
void Algorithm::setResultState(const ResultState state) {
  if (state != ResultState::NotFinished) {
    setExecutionState(ExecutionState::Finished);
  }
  m_resultState = state;
}

//---------------------------------------------------------------------------------------------
/// Has the Algorithm already been initialized
bool Algorithm::isInitialized() const { return (m_executionState != ExecutionState::Uninitialized); }

/// Has the Algorithm already been executed successfully
bool Algorithm::isExecuted() const {
  return ((executionState() == ExecutionState::Finished) && (resultState() == ResultState::Success));
}

//---------------------------------------------------------------------------------------------
/** To query whether algorithm is a child.
 *  @returns true - the algorithm is a child algorithm.  False - this is a full
 * managed algorithm.
 */
bool Algorithm::isChild() const { return m_isChildAlgorithm; }

/** To set whether algorithm is a child.
 *  @param isChild :: True - the algorithm is a child algorithm.  False - this
 * is a full managed algorithm.
 */
void Algorithm::setChild(const bool isChild) {
  m_isChildAlgorithm = isChild;
  this->setAlwaysStoreInADS(!isChild);
}

/**
 * Change the state of the history recording flag. Only applicable for
 * child algorithms.
 * @param on :: The new state of the flag
 */
void Algorithm::enableHistoryRecordingForChild(const bool on) { m_recordHistoryForChild = on; }

/** Do we ALWAYS store in the AnalysisDataService? This is set to true
 * for python algorithms' child algorithms
 *
 * @param doStore :: always store in ADS
 */
void Algorithm::setAlwaysStoreInADS(const bool doStore) {
  m_alwaysStoreInADS = doStore;

  // Set OutputWorkspace as an optional property in the case where alwaysStoreInADS is false. In
  // this case, the output workspace name is not always required.
  if (!m_alwaysStoreInADS && m_properties.existsProperty("OutputWorkspace")) {
    Property *property = m_properties.getPointerToProperty("OutputWorkspace");
    setPropertyModeForWorkspaceProperty(property, PropertyMode::Type::Optional);
  }
}

/** Returns true if we always store in the AnalysisDataService.
 *  @return true if output is saved to the AnalysisDataService.
 */
bool Algorithm::getAlwaysStoreInADS() const { return m_alwaysStoreInADS; }

/** Set whether the algorithm will rethrow exceptions
 * @param rethrow :: true if you want to rethrow exception.
 */
void Algorithm::setRethrows(const bool rethrow) { this->m_rethrow = rethrow; }

/// True if the algorithm is running.
bool Algorithm::isRunning() const { return (executionState() == ExecutionState::Running); }

/// True if the algorithm is ready for garbage collection.
bool Algorithm::isReadyForGarbageCollection() const {
  if ((executionState() == ExecutionState::Finished) &&
      (Mantid::Types::Core::DateAndTime::getCurrentTime() > m_gcTime)) {
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------------------------
/**  Add an observer to a notification
@param observer :: Reference to the observer to add
*/
void Algorithm::addObserver(const Poco::AbstractObserver &observer) const {
  notificationCenter().addObserver(observer);
}

/**  Remove an observer
@param observer :: Reference to the observer to remove
*/
void Algorithm::removeObserver(const Poco::AbstractObserver &observer) const {
  notificationCenter().removeObserver(observer);
}

//---------------------------------------------------------------------------------------------
/** Sends ProgressNotification.
 * @param p :: Reported progress,  must be between 0 (just started) and 1
 * (finished)
 * @param msg :: Optional message string
 * @param estimatedTime :: Optional estimated time to completion
 * @param progressPrecision :: optional, int number of digits after the decimal
 * point to show.
 */
void Algorithm::progress(double p, const std::string &msg, double estimatedTime, int progressPrecision) {
  notificationCenter().postNotification(new ProgressNotification(this, p, msg, estimatedTime, progressPrecision));
}

//---------------------------------------------------------------------------------------------
/// Function to return all of the categories that contain this algorithm
const std::vector<std::string> Algorithm::categories() const {
  Mantid::Kernel::StringTokenizer tokenizer(category(), categorySeparator(),
                                            Mantid::Kernel::StringTokenizer::TOK_TRIM |
                                                Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);

  auto res = tokenizer.asVector();

  const auto *depo = dynamic_cast<const DeprecatedAlgorithm *>(this);
  if (depo != nullptr) {
    res.emplace_back("Deprecated");
  }
  return res;
}

/**
 * @return A string giving the method name that should be attached to a
 * workspace
 */
const std::string Algorithm::workspaceMethodName() const { return ""; }

/**
 *
 * @return A list of workspace class names that should have the
 *workspaceMethodName attached
 */
const std::vector<std::string> Algorithm::workspaceMethodOn() const {
  Mantid::Kernel::StringTokenizer tokenizer(this->workspaceMethodOnTypes(), WORKSPACE_TYPES_SEPARATOR,
                                            Mantid::Kernel::StringTokenizer::TOK_TRIM |
                                                Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  return tokenizer.asVector();
}

/**
 * @return The name of the property that the calling object will be passed to.
 */
const std::string Algorithm::workspaceMethodInputProperty() const { return ""; }

//---------------------------------------------------------------------------------------------
/** Initialization method invoked by the framework. This method is responsible
 *  for any bookkeeping of initialization required by the framework itself.
 *  It will in turn invoke the init() method of the derived algorithm,
 *  and of any Child Algorithms which it creates.
 *  @throw runtime_error Thrown if algorithm or Child Algorithm cannot be
 *initialised
 *
 */
void Algorithm::initialize() {
  // Bypass the initialization if the algorithm has already been initialized.
  if (isInitialized())
    return;

  g_log.setName(this->name());
  setLoggingOffset(0);
  try {
    try {
      this->init();
    } catch (std::runtime_error &) {
      throw;
    }

    // Indicate that this Algorithm has been initialized to prevent duplicate
    // attempts.
    setExecutionState(ExecutionState::Initialized);
  } catch (std::runtime_error &) {
    throw;
  }
  // Unpleasant catch-all! Along with this, Gaudi version catches GaudiException
  // & std::exception
  // but doesn't really do anything except (print fatal) messages.
  catch (...) {
    // Gaudi: A call to the auditor service is here
    // (1) perform the printout
    getLogger().fatal("UNKNOWN Exception is caught in initialize()");
    throw;
  }
}

//---------------------------------------------------------------------------------------------
/** Perform validation of ALL the input properties of the algorithm.
 * This is to be overridden by specific algorithms.
 * It will be called in dialogs after parsing all inputs and setting the
 * properties, but BEFORE executing.
 *
 * @return a map where: Key = string name of the property;
            Value = string describing the problem with the property.
 */
std::map<std::string, std::string> Algorithm::validateInputs() { return std::map<std::string, std::string>(); }

//---------------------------------------------------------------------------------------------
/**
 * Go through the properties and cache the input/output
 * workspace properties for later use.
 */
void Algorithm::cacheWorkspaceProperties() {
  m_inputWorkspaceProps.clear();
  m_outputWorkspaceProps.clear();
  m_pureOutputWorkspaceProps.clear();
  const auto &props = this->getProperties();
  for (const auto &prop : props) {
    auto wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
    if (!wsProp)
      continue;
    switch (prop->direction()) {
    case Kernel::Direction::Input:
      m_inputWorkspaceProps.emplace_back(wsProp);
      break;
    case Kernel::Direction::InOut:
      m_inputWorkspaceProps.emplace_back(wsProp);
      m_outputWorkspaceProps.emplace_back(wsProp);
      break;
    case Kernel::Direction::Output:
      m_outputWorkspaceProps.emplace_back(wsProp);
      m_pureOutputWorkspaceProps.emplace_back(wsProp);
      break;
    default:
      throw std::logic_error("Unexpected property direction found for property " + prop->name() + " of algorithm " +
                             this->name());
    }
  }
}

/**
 * Cache the histories of any input workspaces so they can be copied over after
 * algorithm completion.
 */
void Algorithm::cacheInputWorkspaceHistories() {
  if (!trackingHistory())
    return;

  auto cacheHistories = [this](const Workspace_sptr &ws) {
    if (auto group = dynamic_cast<const WorkspaceGroup *>(ws.get())) {
      m_inputWorkspaceHistories.reserve(m_inputWorkspaceHistories.size() + group->size());
      std::copy(group->begin(), group->end(), std::back_inserter(m_inputWorkspaceHistories));
    } else {
      m_inputWorkspaceHistories.emplace_back(ws);
    }
  };
  using ArrayPropertyString = ArrayProperty<std::string>;
  auto isADSValidator = [](const IValidator_sptr &validator) -> bool {
    if (!validator)
      return false;
    if (dynamic_cast<ADSValidator *>(validator.get()))
      return true;
    if (const auto compValidator = dynamic_cast<CompositeValidator *>(validator.get()))
      return compValidator->contains<ADSValidator>();

    return false;
  };

  // Look over all properties so we can catch an string array properties
  // with an ADSValidator. ADSValidator indicates that the strings
  // point to workspace names so we want to pick up the history from these too.
  const auto &ads = AnalysisDataService::Instance();
  m_inputWorkspaceHistories.clear();
  const auto &props = this->getProperties();
  for (const auto &prop : props) {
    if (prop->direction() != Direction::Input && prop->direction() != Direction::InOut)
      continue;

    if (auto wsProp = dynamic_cast<IWorkspaceProperty *>(prop)) {
      if (auto ws = wsProp->getWorkspace()) {
        cacheHistories(ws);
      } else {
        Workspace_sptr wsFromADS;
        try {
          wsFromADS = ads.retrieve(prop->value());
        } catch (Exception::NotFoundError &) {
          continue;
        }
        cacheHistories(wsFromADS);
      }
    } else if (auto strArrayProp = dynamic_cast<ArrayPropertyString *>(prop)) {
      if (!isADSValidator(strArrayProp->getValidator()))
        continue;
      const auto &wsNames((*strArrayProp)());
      for (const auto &wsName : wsNames) {
        cacheHistories(ads.retrieve(wsName));
      }
    }
  }
} // namespace API

//---------------------------------------------------------------------------------------------
/** Go through the workspace properties of this algorithm
 * and lock the workspaces for reading or writing.
 *
 */
void Algorithm::lockWorkspaces() {
  // Do not lock workspace for child algos
  if (this->isChild())
    return;

  if (!m_readLockedWorkspaces.empty() || !m_writeLockedWorkspaces.empty())
    throw std::logic_error("Algorithm::lockWorkspaces(): The workspaces have "
                           "already been locked!");

  // First, Write-lock the output workspaces
  auto &debugLog = g_log.debug();
  for (auto &outputWorkspaceProp : m_outputWorkspaceProps) {
    Workspace_sptr ws = outputWorkspaceProp->getWorkspace();
    if (ws) {
      // The workspace property says to do locking,
      // AND it has NOT already been write-locked
      if (outputWorkspaceProp->isLocking() && std::find(m_writeLockedWorkspaces.begin(), m_writeLockedWorkspaces.end(),
                                                        ws) == m_writeLockedWorkspaces.end()) {
        // Write-lock it if not already
        debugLog << "Write-locking " << ws->getName() << '\n';
        ws->getLock()->writeLock();
        m_writeLockedWorkspaces.emplace_back(ws);
      }
    }
  }

  // Next read-lock the input workspaces
  for (auto &inputWorkspaceProp : m_inputWorkspaceProps) {
    Workspace_sptr ws = inputWorkspaceProp->getWorkspace();
    if (ws) {
      // The workspace property says to do locking,
      // AND it has NOT already been write-locked
      if (inputWorkspaceProp->isLocking() && std::find(m_writeLockedWorkspaces.begin(), m_writeLockedWorkspaces.end(),
                                                       ws) == m_writeLockedWorkspaces.end()) {
        // Read-lock it if not already write-locked
        debugLog << "Read-locking " << ws->getName() << '\n';
        ws->getLock()->readLock();
        m_readLockedWorkspaces.emplace_back(ws);
      }
    }
  }
}

//---------------------------------------------------------------------------------------------
/** Unlock any previously locked workspaces
 *
 */
void Algorithm::unlockWorkspaces() {
  // Do not lock workspace for child algos
  if (this->isChild())
    return;
  auto &debugLog = g_log.debug();
  for (auto &ws : m_writeLockedWorkspaces) {
    if (ws) {
      debugLog << "Unlocking " << ws->getName() << '\n';
      ws->getLock()->unlock();
    }
  }
  for (auto &ws : m_readLockedWorkspaces) {
    if (ws) {
      debugLog << "Unlocking " << ws->getName() << '\n';
      ws->getLock()->unlock();
    }
  }

  // Don't double-unlock workspaces
  m_readLockedWorkspaces.clear();
  m_writeLockedWorkspaces.clear();
}

/**
 * Clear any internal workspace handles so that workspaces will be deleted
 * promptly after a managed algorithm finishes
 */
void Algorithm::clearWorkspaceCaches() {
  m_groupWorkspaces.clear();
  m_inputWorkspaceHistories.clear();
  m_inputWorkspaceProps.clear();
  m_outputWorkspaceProps.clear();
  m_pureOutputWorkspaceProps.clear();
  m_unrolledInputWorkspaces.clear();
}

//---------------------------------------------------------------------------------------------
/** Invoced internally in execute()
 */

bool Algorithm::executeInternal() {
  Timer timer;
  bool algIsExecuted = false;
  AlgorithmManager::Instance().notifyAlgorithmStarting(this->getAlgorithmID());

  // runtime check for deprecation warning
  {
    auto *depo = dynamic_cast<DeprecatedAlgorithm *>(this);
    if (depo != nullptr)
      getLogger().error(depo->deprecationMsg(this));
  }

  // runtime check for deprecated alias warning
  {
    auto *da_alg = dynamic_cast<DeprecatedAlias *>(this);
    if ((da_alg != nullptr) && (this->calledByAlias))
      getLogger().warning(da_alg->deprecationMessage(this));
  }

  // Register clean up tasks that should happen regardless of the route
  // out of the algorithm. These tasks will get run after this method
  // finishes.
  RunOnFinish onFinish([this]() { this->clearWorkspaceCaches(); });

  notificationCenter().postNotification(new StartedNotification(this));
  Mantid::Types::Core::DateAndTime startTime;

  // Return a failure if the algorithm hasn't been initialized
  if (!isInitialized()) {
    throw std::runtime_error("Algorithm is not initialised:" + this->name());
  }

  // no logging of input if a child algorithm (except for python child algos)
  if (!m_isChildAlgorithm || m_alwaysStoreInADS)
    logAlgorithmInfo();

  // Check all properties for validity
  constexpr bool resetTimer{true};
  float timingInit = timer.elapsed(resetTimer);
  if (!validateProperties()) {
    // Reset name on input workspaces to trigger attempt at collection from ADS
    const auto &props = getProperties();
    for (auto &prop : props) {
      const auto *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
      if (wsProp && !(wsProp->getWorkspace())) {
        // Setting it's name to the same one it already had
        prop->setValue(prop->value());
      }
    }
    // Try the validation again
    if (!validateProperties()) {
      notificationCenter().postNotification(new ErrorNotification(this, "Some invalid Properties found"));
      throw std::runtime_error("Some invalid Properties found");
    }
  }
  const float timingPropertyValidation = timer.elapsed(resetTimer);

  // All properties are now valid - cache workspace properties and histories
  cacheWorkspaceProperties();
  cacheInputWorkspaceHistories();

  // ----- Check for processing groups -------------
  // default true so that it has the right value at the check below the catch
  // block should checkGroups throw
  bool callProcessGroups = true;
  try {
    // Checking the input is a group. Throws if the sizes are wrong
    callProcessGroups = this->checkGroups();
  } catch (std::exception &ex) {
    getLogger().error() << "Error in execution of algorithm " << this->name() << "\n" << ex.what() << "\n";
    notificationCenter().postNotification(new ErrorNotification(this, ex.what()));
    setResultState(ResultState::Failed);
    if (m_isChildAlgorithm || m_runningAsync || m_rethrow) {
      m_runningAsync = false;
      throw;
    }
    return false;
  }

  timingInit += timer.elapsed(resetTimer);
  // ----- Perform validation of the whole set of properties -------------
  if ((!callProcessGroups)) // for groups this is called on each workspace separately
  {
    std::map<std::string, std::string> errors = this->validateInputs();
    if (!errors.empty()) {
      size_t numErrors = errors.size();
      // Log each issue
      auto &errorLog = getLogger().error();
      auto &warnLog = getLogger().warning();
      for (auto &error : errors) {
        if (this->existsProperty(error.first))
          errorLog << "Invalid value for " << error.first << ": " << error.second << "\n";
        else {
          numErrors -= 1; // don't count it as an error
          warnLog << "validateInputs() references non-existant property \"" << error.first << "\"\n";
        }
      }
      // Throw because something was invalid
      if (numErrors > 0) {
        std::stringstream msg;
        msg << "Some invalid Properties found: ";
        for (const auto &error : errors) {
          msg << "\n " << error.first << ": " << error.second;
        }
        notificationCenter().postNotification(new ErrorNotification(this, "Some invalid Properties found"));
        throw std::runtime_error(msg.str());
      }
    }
  }
  const float timingInputValidation = timer.elapsed(resetTimer);

  if (trackingHistory()) {
    // count used for defining the algorithm execution order
    // If history is being recorded we need to count this as a separate
    // algorithm
    // as the history compares histories by their execution number
    ++Algorithm::g_execCount;

    // populate history record before execution so we can record child
    // algorithms in it
    AlgorithmHistory algHist;
    m_history = std::make_shared<AlgorithmHistory>(algHist);
  }

  // ----- Process groups -------------
  // If checkGroups() threw an exception but there ARE group workspaces
  // (means that the group sizes were incompatible)
  if (callProcessGroups) {
    return doCallProcessGroups(startTime);
  }

  // Read or write locks every input/output workspace
  this->lockWorkspaces();
  timingInit += timer.elapsed(resetTimer);

  // Invoke exec() method of derived class and catch all uncaught exceptions
  try {
    try {
      setExecutionState(ExecutionState::Running);

      startTime = Mantid::Types::Core::DateAndTime::getCurrentTime();
      // Call the concrete algorithm's exec method
      this->exec();
      registerFeatureUsage();
      // Check for a cancellation request in case the concrete algorithm doesn't
      interruption_point();
      const float timingExec = timer.elapsed(resetTimer);
      // The total runtime including all init steps is used for general logging.
      const float duration = timingInit + timingPropertyValidation + timingInputValidation + timingExec;
      // need it to throw before trying to run fillhistory() on an algorithm
      // which has failed
      if (trackingHistory() && m_history) {
        m_history->fillAlgorithmHistory(this, startTime, duration, Algorithm::g_execCount);
        fillHistory();
        linkHistoryWithLastChild();
      }

      // Put the output workspaces into the AnalysisDataService - if requested
      if (m_alwaysStoreInADS)
        this->store();

      // just cache the value internally, it is set at the very end of this
      // method
      algIsExecuted = true;

      // Log that execution has completed.
      getLogger().debug("Time to validate properties: " + std::to_string(timingPropertyValidation) + " seconds\n" +
                        "Time for other input validation: " + std::to_string(timingInputValidation) + " seconds\n" +
                        "Time for other initialization: " + std::to_string(timingInit) + " seconds\n" +
                        "Time to run exec: " + std::to_string(timingExec) + " seconds\n");
      reportCompleted(duration);
    } catch (std::runtime_error &ex) {
      m_gcTime = Mantid::Types::Core::DateAndTime::getCurrentTime() +=
          (Mantid::Types::Core::DateAndTime::ONE_SECOND * DELAY_BEFORE_GC);
      setResultState(ResultState::Failed);
      this->unlockWorkspaces();
      if (m_isChildAlgorithm || m_runningAsync || m_rethrow)
        throw;
      else {
        notificationCenter().postNotification(new ErrorNotification(this, ex.what()));
        getLogger().error() << "Error in execution of algorithm " << this->name() << '\n' << ex.what() << '\n';
      }

    } catch (std::logic_error &ex) {
      m_gcTime = Mantid::Types::Core::DateAndTime::getCurrentTime() +=
          (Mantid::Types::Core::DateAndTime::ONE_SECOND * DELAY_BEFORE_GC);
      notificationCenter().postNotification(new ErrorNotification(this, ex.what()));
      setResultState(ResultState::Failed);
      this->unlockWorkspaces();
      if (m_isChildAlgorithm || m_runningAsync || m_rethrow)
        throw;
      else {
        getLogger().error() << "Logic Error in execution of algorithm " << this->name() << '\n' << ex.what() << '\n';
      }
    }
  } catch (CancelException &ex) {
    m_runningAsync = false;
    getLogger().warning() << this->name() << ": Execution cancelled by user.\n";
    m_gcTime = Mantid::Types::Core::DateAndTime::getCurrentTime() +=
        (Mantid::Types::Core::DateAndTime::ONE_SECOND * DELAY_BEFORE_GC);
    setResultState(ResultState::Failed);
    notificationCenter().postNotification(new ErrorNotification(this, ex.what()));
    this->unlockWorkspaces();

    throw;
  }
  // Gaudi also specifically catches GaudiException & std:exception.
  catch (std::exception &ex) {
    m_runningAsync = false;
    getLogger().error() << "Error in execution of algorithm " << this->name() << ":\n" << ex.what() << "\n";
    m_gcTime = Mantid::Types::Core::DateAndTime::getCurrentTime() +=
        (Mantid::Types::Core::DateAndTime::ONE_SECOND * DELAY_BEFORE_GC);
    setResultState(ResultState::Failed);
    notificationCenter().postNotification(new ErrorNotification(this, ex.what()));
    this->unlockWorkspaces();

    throw;
  }

  catch (H5::Exception &ex) {
    m_runningAsync = false;
    std::string errmsg;
    errmsg.append(ex.getCFuncName()).append(": ").append(ex.getCDetailMsg());
    getLogger().error() << "H5 Exception in execution of algorithm " << this->name() << ":\n" << errmsg << "\n";
    m_gcTime = Mantid::Types::Core::DateAndTime::getCurrentTime() +=
        (Mantid::Types::Core::DateAndTime::ONE_SECOND * DELAY_BEFORE_GC);
    setResultState(ResultState::Failed);
    notificationCenter().postNotification(new ErrorNotification(this, errmsg));
    this->unlockWorkspaces();

    throw;
  }

  catch (...) {
    // Execution failed with an unknown exception object
    m_runningAsync = false;

    m_gcTime = Mantid::Types::Core::DateAndTime::getCurrentTime() +=
        (Mantid::Types::Core::DateAndTime::ONE_SECOND * DELAY_BEFORE_GC);
    setResultState(ResultState::Failed);
    notificationCenter().postNotification(new ErrorNotification(this, "UNKNOWN Exception is caught in exec()"));
    getLogger().error() << this->name() << ": UNKNOWN Exception is caught in exec()\n";
    this->unlockWorkspaces();

    throw;
  }

  m_gcTime = Mantid::Types::Core::DateAndTime::getCurrentTime() +=
      (Mantid::Types::Core::DateAndTime::ONE_SECOND * DELAY_BEFORE_GC);
  if (algIsExecuted) {
    setResultState(ResultState::Success);
  }

  // Only gets to here if algorithm ended normally
  notificationCenter().postNotification(new FinishedNotification(this, isExecuted()));

  // Unlock the workspaces once the notification has been sent, to prevent too early deletion
  this->unlockWorkspaces();

  return isExecuted();
}

//---------------------------------------------------------------------------------------------
/** Execute as a Child Algorithm.
 * This runs execute() but catches errors so as to log the name
 * of the failed Child Algorithm, if it fails.
 */
void Algorithm::executeAsChildAlg() {
  bool executed = false;
  try {
    executed = execute();
  } catch (std::runtime_error &) {
    throw;
  }

  if (!executed) {
    throw std::runtime_error("Unable to successfully run ChildAlgorithm " + this->name());
  }
}

//---------------------------------------------------------------------------------------------
/** Stores any output workspaces into the AnalysisDataService
 *  @throw std::runtime_error If unable to successfully store an output
 * workspace
 */
void Algorithm::store() {
  const std::vector<Property *> &props = getProperties();
  std::vector<int> groupWsIndicies;

  // add any regular/child workspaces first, then add the groups
  for (unsigned int i = 0; i < props.size(); ++i) {
    auto *wsProp = dynamic_cast<IWorkspaceProperty *>(props[i]);
    if (wsProp) {
      // check if the workspace is a group, if so remember where it is and add
      // it later
      auto group = std::dynamic_pointer_cast<WorkspaceGroup>(wsProp->getWorkspace());
      if (!group) {
        try {
          wsProp->store();
        } catch (std::runtime_error &) {
          throw;
        }
      } else {
        groupWsIndicies.emplace_back(i);
      }
    }
  }

  // now store workspace groups once their members have been added
  std::vector<int>::const_iterator wsIndex;
  for (wsIndex = groupWsIndicies.begin(); wsIndex != groupWsIndicies.end(); ++wsIndex) {
    auto *wsProp = dynamic_cast<IWorkspaceProperty *>(props[*wsIndex]);
    if (wsProp) {
      try {
        wsProp->store();
      } catch (std::runtime_error &) {
        throw;
      }
    }
  }
}

//---------------------------------------------------------------------------------------------
/** Create a Child Algorithm.  A call to this method creates a child algorithm
 *object.
 *  Using this mechanism instead of creating daughter
 *  algorithms directly via the new operator is prefered since then
 *  the framework can take care of all of the necessary book-keeping.
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
Algorithm_sptr Algorithm::createChildAlgorithm(const std::string &name, const double startProgress,
                                               const double endProgress, const bool enableLogging, const int &version) {
  Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(name, version);
  setupAsChildAlgorithm(alg, startProgress, endProgress, enableLogging);
  return alg;
}

/** Setup algorithm as child algorithm.
 *
 * Used internally by createChildAlgorithm. Arguments are as documented there.
 * Can also be used manually for algorithms created otherwise. This allows
 * running algorithms that are not declared into the factory as child
 * algorithms. */
void Algorithm::setupAsChildAlgorithm(const Algorithm_sptr &alg, const double startProgress, const double endProgress,
                                      const bool enableLogging) {
  // set as a child
  alg->setChild(true);
  alg->setLogging(enableLogging);

  // Initialise the Child Algorithm
  try {
    alg->initialize();
  } catch (std::runtime_error &) {
    throw std::runtime_error("Unable to initialise Child Algorithm '" + alg->name() + "'");
  }

  // If output workspaces are nameless, give them a temporary name to satisfy
  // validator
  const std::vector<Property *> &props = alg->getProperties();
  for (auto prop : props) {
    const auto *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
    if (prop->direction() == Mantid::Kernel::Direction::Output && wsProp) {
      if (prop->value().empty() && !wsProp->isOptional()) {
        prop->createTemporaryValue();
      }
    }
  }

  if (startProgress >= 0.0 && endProgress > startProgress && endProgress <= 1.0) {
    alg->addObserver(this->progressObserver());
    m_startChildProgress = startProgress;
    m_endChildProgress = endProgress;
  }

  // Before we return the shared pointer, use it to create a weak pointer and
  // keep that in a vector.
  // It will be used this to pass on cancellation requests
  // It must be protected by a critical block so that Child Algorithms can run
  // in parallel safely.
  std::weak_ptr<IAlgorithm> weakPtr(alg);
  PARALLEL_CRITICAL(Algorithm_StoreWeakPtr) { m_ChildAlgorithms.emplace_back(weakPtr); }
}

//=============================================================================================
//================================== Algorithm History
//========================================
//=============================================================================================

/**
 * Serialize this object to a string. The format is
 * a json formatted string.
 * @returns This object serialized as a string
 */
std::string Algorithm::toString() const { return Mantid::JsonHelpers::jsonToString(toJson()); }

/**
 * Serialize this object to a json object)
 * @returns This object serialized as a json object
 */
::Json::Value Algorithm::toJson() const {
  ::Json::Value root;

  root["name"] = name();
  root["version"] = this->version();
  root["properties"] = m_properties.asJson(false);

  return root;
}

//--------------------------------------------------------------------------------------------
/** Construct an object from a history entry.
 *
 * This creates the algorithm and sets all of its properties using the history.
 *
 * @param history :: AlgorithmHistory object
 * @return a shared pointer to the created algorithm.
 */
IAlgorithm_sptr Algorithm::fromHistory(const AlgorithmHistory &history) {
  ::Json::Value root;
  ::Json::Value jsonMap;

  auto props = history.getProperties();
  const size_t numProps(props.size());
  for (size_t i = 0; i < numProps; ++i) {
    PropertyHistory_sptr prop = props[i];
    if (!prop->isDefault()) {
      jsonMap[prop->name()] = prop->value();
    }
  }

  root["name"] = history.name();
  root["version"] = history.version();
  root["properties"] = jsonMap;

  const std::string output = Mantid::JsonHelpers::jsonToString(root);
  IAlgorithm_sptr alg;

  try {
    alg = Algorithm::fromString(output);
  } catch (std::invalid_argument &) {
    throw std::runtime_error("Could not create algorithm from history. "
                             "Is this a child algorithm whose workspaces are not in the ADS?");
  }
  return alg;
}

//--------------------------------------------------------------------------------------------
/** De-serializes the algorithm from a string
 *
 * @param input :: An input string in the format. The format is
 * AlgorithmName.version(prop1=value1,prop2=value2,...). If .version is
 * not found the highest found is used.
 * @return A pointer to a managed algorithm object
 * @throws std::runtime_error if the algorithm cannot be created
 */
IAlgorithm_sptr Algorithm::fromString(const std::string &input) {
  ::Json::Value root;
  if (Mantid::JsonHelpers::parse(input, &root)) {
    return fromJson(root);
  } else {
    throw std::runtime_error("Cannot create algorithm, invalid string format.");
  }
}

/**
 * De-serializes the algorithm from a Json object
 * @param serialized A reference to Json::Value that contains a serialized
 * algorithm object
 * @return A new algorithm object
 * @throws std::runtime_error if the algorithm cannot be created
 */
IAlgorithm_sptr Algorithm::fromJson(const Json::Value &serialized) {
  const std::string algName = serialized["name"].asString();
  const int versionNumber = serialized.get("version", -1).asInt();
  auto alg = AlgorithmManager::Instance().createUnmanaged(algName, versionNumber);
  alg->initialize();
  alg->setProperties(serialized["properties"]);
  return alg;
}

/** Fills History, Algorithm History and Algorithm Parameters
 */
void Algorithm::fillHistory() {
  WorkspaceVector outputWorkspaces;
  if (!isChild()) {
    findWorkspaces(outputWorkspaces, Direction::Output);
  }
  fillHistory(outputWorkspaces);
}

/**
 * Link the name of the output workspaces on this parent algorithm.
 * with the last child algorithm executed to ensure they match in the history.
 *
 * This solves the case where child algorithms use a temporary name and this
 * name needs to match the output name of the parent algorithm so the history
 *can be re-run.
 */
void Algorithm::linkHistoryWithLastChild() {
  if (!m_recordHistoryForChild)
    return;

  // iterate over the algorithms output workspaces
  const auto &algProperties = getProperties();
  for (const auto &prop : algProperties) {
    if (prop->direction() != Kernel::Direction::Output && prop->direction() != Kernel::Direction::InOut)
      continue;
    const auto *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
    if (!wsProp)
      continue;
    // Check we actually have a workspace, it may have been optional
    Workspace_sptr workspace = wsProp->getWorkspace();
    if (!workspace)
      continue;

    bool linked = false;
    // find child histories with anonymous output workspaces
    const auto &childHistories = m_history->getChildHistories();
    auto childIter = childHistories.rbegin();
    for (; childIter != childHistories.rend() && !linked; ++childIter) {
      const auto &props = (*childIter)->getProperties();
      auto propIter = props.begin();
      for (; propIter != props.end() && !linked; ++propIter) {
        // check we have a workspace property
        if ((*propIter)->direction() == Kernel::Direction::Output ||
            (*propIter)->direction() == Kernel::Direction::InOut) {
          // if the workspaces are equal, then rename the history
          std::ostringstream os;
          os << "__TMP" << wsProp->getWorkspace().get();
          if (os.str() == (*propIter)->value()) {
            (*propIter)->setValue(prop->value());
            linked = true;
          }
        }
      }
    }
  }
}

/** Indicates that this algrithms history should be tracked regardless of if it
 * is a child.
 *  @param parentHist :: the parent algorithm history object the history in.
 */
void Algorithm::trackAlgorithmHistory(std::shared_ptr<AlgorithmHistory> parentHist) {
  enableHistoryRecordingForChild(true);
  m_parentHistory = std::move(parentHist);
}

/** Check if we are tracking history for this algorithm
 *  @return if we are tracking the history of this algorithm
 */
bool Algorithm::trackingHistory() { return (!isChild() || m_recordHistoryForChild); }

/** Populate lists of the workspace properties for a given direction
 *  (InOut workspaces are included in both input/output)
 * @param workspaces A reference to a vector for the workspaces
 * @param direction The direction of the property required for the search
 * @param checkADS If true, check the ADS for workspace references
 * if the check on the workspace property value is empty. Most useful for
 * finding group workspaces that are never stored on the property
 */
void Algorithm::findWorkspaces(WorkspaceVector &workspaces, unsigned int direction, bool checkADS) const {
  auto workspaceFromWSProperty = [](const IWorkspaceProperty &prop, const AnalysisDataServiceImpl &ads,
                                    const std::string &strValue, bool checkADS) {
    auto workspace = prop.getWorkspace();
    if (workspace)
      return workspace;

    // Empty string indicates optional workspace
    if (checkADS && !strValue.empty()) {
      return ads.retrieve(strValue);
    }
    return Workspace_sptr();
  };
  auto appendWS = [&workspaces](const Workspace_sptr &workspace) {
    if (!workspace)
      return false;
    workspaces.emplace_back(workspace);
    return true;
  };

  // Additional output properties can be declared on the fly
  // so we need a fresh loop over the properties
  const auto &algProperties = getProperties();
  const auto &ads = AnalysisDataService::Instance();
  for (const auto &prop : algProperties) {
    const unsigned int propDirection = prop->direction();
    if (propDirection != direction && propDirection != Direction::InOut)
      continue;
    if (const auto wsProp = dynamic_cast<IWorkspaceProperty *>(prop)) {
      appendWS(workspaceFromWSProperty(*wsProp, ads, prop->value(), checkADS));
    }
  }
}

/** Sends out algorithm parameter information to the logger */
void Algorithm::logAlgorithmInfo() const {
  auto &logger = getLogger();

  if (m_isAlgStartupLoggingEnabled) {
    logger.notice() << name() << " started";
    if (this->isChild())
      logger.notice() << " (child)";
    logger.notice() << '\n';
    // Make use of the AlgorithmHistory class, which holds all the info we
    // want here
    AlgorithmHistory algHistory(this);
    size_t maxPropertyLength = 40;
    if (logger.is(Logger::Priority::PRIO_DEBUG)) {
      // include the full property value when logging in debug
      maxPropertyLength = 0;
    }
    algHistory.printSelf(logger.information(), 0, maxPropertyLength);
  }
}

//=============================================================================================
//================================== WorkspaceGroup-related
//===================================
//=============================================================================================

/** Check the input workspace properties for groups.
 *
 * If there are more than one input workspace properties, then:
 *  - All inputs should be groups of the same size
 *    - In this case, algorithms are processed in order
 *  - OR, only one input should be a group, the others being size of 1
 *
 * If the property itself is a WorkspaceProperty<WorkspaceGroup> then
 * this returns false
 *
 * Returns true if processGroups() should be called.
 * It also sets up some other members.
 *
 * Override if it is needed to customize the group checking.
 *
 * @throw std::invalid_argument if the groups sizes are incompatible.
 * @throw std::invalid_argument if a member is not found
 *
 * This method (or an override) must NOT THROW any exception if there are no
 *input workspace groups
 */
bool Algorithm::checkGroups() {
  size_t numGroups = 0;
  bool doProcessGroups = false;

  // Unroll the groups or single inputs into vectors of workspaces
  const auto &ads = AnalysisDataService::Instance();
  m_unrolledInputWorkspaces.clear();
  m_groupWorkspaces.clear();
  for (auto inputWorkspaceProp : m_inputWorkspaceProps) {
    auto prop = dynamic_cast<Property *>(inputWorkspaceProp);
    auto wsGroupProp = dynamic_cast<WorkspaceProperty<WorkspaceGroup> *>(prop);
    auto ws = inputWorkspaceProp->getWorkspace();
    auto wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(ws);

    // Workspace groups are NOT returned by IWP->getWorkspace() most of the
    // time because WorkspaceProperty is templated by <MatrixWorkspace> and
    // WorkspaceGroup does not subclass <MatrixWorkspace>
    if (!wsGroup && prop && !prop->value().empty()) {
      // So try to use the name in the AnalysisDataService
      try {
        wsGroup = ads.retrieveWS<WorkspaceGroup>(prop->value());
      } catch (Exception::NotFoundError &) { /* Do nothing */
      }
    }

    // Found the group either directly or by name?
    // If the property is of type WorkspaceGroup then don't unroll
    if (wsGroup && !wsGroupProp) {
      numGroups++;
      doProcessGroups = true;
      m_unrolledInputWorkspaces.emplace_back(wsGroup->getAllItems());
    } else {
      // Single Workspace. Treat it as a "group" with only one member
      if (ws)
        m_unrolledInputWorkspaces.emplace_back(WorkspaceVector{ws});
      else
        m_unrolledInputWorkspaces.emplace_back(WorkspaceVector{});
    }

    // Add to the list of groups
    m_groupWorkspaces.emplace_back(wsGroup);
  }

  // No groups? Get out.
  if (numGroups == 0)
    return doProcessGroups;

  // ---- Confirm that all the groups are the same size -----
  // Index of the single group
  m_singleGroup = -1;
  // Size of the single or of all the groups
  m_groupSize = 1;
  m_groupsHaveSimilarNames = true;
  for (size_t i = 0; i < m_unrolledInputWorkspaces.size(); i++) {
    const auto &thisGroup = m_unrolledInputWorkspaces[i];
    // We're ok with empty groups if the workspace property is optional
    if (thisGroup.empty() && !m_inputWorkspaceProps[i]->isOptional())
      throw std::invalid_argument("Empty group passed as input");
    if (!thisGroup.empty()) {
      // Record the index of the single group.
      WorkspaceGroup_sptr wsGroup = m_groupWorkspaces[i];
      if (wsGroup && (numGroups == 1))
        m_singleGroup = int(i);

      // For actual groups (>1 members)
      if (thisGroup.size() > 1) {
        // Check for matching group size
        if (m_groupSize > 1)
          if (thisGroup.size() != m_groupSize)
            throw std::invalid_argument("Input WorkspaceGroups are not of the same size.");

        // Are ALL the names similar?
        if (wsGroup)
          m_groupsHaveSimilarNames = m_groupsHaveSimilarNames && wsGroup->areNamesSimilar();

        // Save the size for the next group
        m_groupSize = thisGroup.size();
      }
    }
  } // end for each group

  // If you get here, then the groups are compatible
  return doProcessGroups;
}

/**
 * Calls process groups with the required timing checks and algorithm
 * execution finalization steps.
 *
 * @param startTime to record the algorithm execution start
 *
 * @return whether processGroups succeeds.
 */
bool Algorithm::doCallProcessGroups(Mantid::Types::Core::DateAndTime &startTime) {
  // In the base implementation of processGroups, this normally calls
  // this->execute() again on each member of the group. Other algorithms may
  // choose to override that behavior (examples: CompareWorkspaces,
  // RenameWorkspace)

  startTime = Mantid::Types::Core::DateAndTime::getCurrentTime();
  // Start a timer
  Timer timer;
  bool completed = false;
  try {
    // Call the concrete algorithm's processGroups method
    completed = processGroups();
  } catch (std::exception &ex) {
    // The child algorithm will already have logged the error etc.,
    // but we also need to update flags in the parent algorithm and
    // send an ErrorNotification (because the child isn't registered with the
    // AlgorithmMonitor).
    setResultState(ResultState::Failed);
    m_runningAsync = false;
    notificationCenter().postNotification(new ErrorNotification(this, ex.what()));
    throw;
  } catch (...) {
    setResultState(ResultState::Failed);
    m_runningAsync = false;
    notificationCenter().postNotification(new ErrorNotification(this, "UNKNOWN Exception caught from processGroups"));
    throw;
  }

  // Check for a cancellation request in case the concrete algorithm doesn't
  interruption_point();

  if (completed) {
    // Get how long this algorithm took to run
    const float duration = timer.elapsed();

    m_history = std::make_shared<AlgorithmHistory>(this, startTime, duration, ++g_execCount);
    if (trackingHistory() && m_history) {
      // find any further outputs created by the execution
      WorkspaceVector outputWorkspaces;
      const bool checkADS{true};
      findWorkspaces(outputWorkspaces, Direction::Output, checkADS);
      fillHistory(outputWorkspaces);
    }

    // in the base processGroups each individual exec stores its outputs
    if (!m_usingBaseProcessGroups && m_alwaysStoreInADS)
      this->store();

    // Log that execution has completed.
    reportCompleted(duration, true /* this is for group processing*/);
    setResultState(ResultState::Success);
  } else {
    setResultState(ResultState::Failed);
  }

  notificationCenter().postNotification(new FinishedNotification(this, isExecuted()));

  return completed;
}

/**
 * If this algorithm is not a child then copy history between the inputs and
 * outputs and add a record for this algorithm. If the algorithm is a child
 * attach the child history to the parent if requested.
 *  @param outputWorkspaces :: A reference to a vector for the output
 * workspaces. Used in the non-child case.
 */
void Algorithm::fillHistory(const std::vector<Workspace_sptr> &outputWorkspaces) {
  // this is not a child algorithm. Add the history algorithm to the
  // WorkspaceHistory object.
  if (!isChild()) {
    auto copyHistoryToGroup = [](const Workspace &in, WorkspaceGroup &out) {
      for (auto &outGroupItem : out) {
        outGroupItem->history().addHistory(in.getHistory());
      }
    };

    for (auto &outWS : outputWorkspaces) {
      auto outWSGroup = std::dynamic_pointer_cast<WorkspaceGroup>(outWS);
      // Copy the history from the cached input workspaces to the output ones
      for (const auto &inputWS : m_inputWorkspaceHistories) {
        if (outWSGroup) {
          copyHistoryToGroup(*inputWS, *outWSGroup);
        } else {
          outWS->history().addHistory(inputWS->getHistory());
        }
      }
      // Add history for this operation
      if (outWSGroup) {
        for (auto &outGroupItem : *outWSGroup) {
          outGroupItem->history().addHistory(m_history);
        }
      } else {
        // Add the history for the current algorithm to all the output
        // workspaces
        outWS->history().addHistory(m_history);
      }
    }
  }
  // this is a child algorithm, but we still want to keep the history.
  else if (m_recordHistoryForChild && m_parentHistory) {
    m_parentHistory->addChildHistory(m_history);
  }
}

//--------------------------------------------------------------------------------------------
/** Process WorkspaceGroup inputs.
 *
 * This should be called after checkGroups(), which sets up required members.
 * It goes through each member of the group(s), creates and sets an algorithm
 * for each and executes them one by one.
 *
 * If there are several group input workspaces, then the member of each group
 * is executed pair-wise.
 *
 * @return true - if all the workspace members are executed.
 */
bool Algorithm::processGroups() {
  m_usingBaseProcessGroups = true;

  std::vector<WorkspaceGroup_sptr> outGroups;

  // ---------- Create all the output workspaces ----------------------------
  for (auto &pureOutputWorkspaceProp : m_pureOutputWorkspaceProps) {
    auto *prop = dynamic_cast<Property *>(pureOutputWorkspaceProp);
    if (prop && !prop->value().empty()) {
      auto outWSGrp = std::make_shared<WorkspaceGroup>();
      outGroups.emplace_back(outWSGrp);
      // Put the GROUP in the ADS
      AnalysisDataService::Instance().addOrReplace(prop->value(), outWSGrp);
      outWSGrp->observeADSNotifications(false);
    }
  }

  double progress_proportion = 1.0 / static_cast<double>(m_groupSize);
  // Go through each entry in the input group(s)
  for (size_t entry = 0; entry < m_groupSize; entry++) {
    // use create Child Algorithm that look like this one
    Algorithm_sptr alg_sptr = this->createChildAlgorithm(this->name(), progress_proportion * static_cast<double>(entry),
                                                         progress_proportion * (1 + static_cast<double>(entry)),
                                                         this->isLogging(), this->version());
    // Make a child algorithm and turn off history recording for it, but always
    // store result in the ADS
    alg_sptr->setChild(true);
    alg_sptr->setAlwaysStoreInADS(true);
    alg_sptr->enableHistoryRecordingForChild(false);
    alg_sptr->setRethrows(true);

    Algorithm *alg = alg_sptr.get();
    // Set all non-workspace properties
    this->copyNonWorkspaceProperties(alg, int(entry) + 1);

    std::string outputBaseName;

    // ---------- Set all the input workspaces ----------------------------
    for (size_t iwp = 0; iwp < m_unrolledInputWorkspaces.size(); iwp++) {
      const std::vector<Workspace_sptr> &thisGroup = m_unrolledInputWorkspaces[iwp];
      if (!thisGroup.empty()) {
        // By default (for a single group) point to the first/only workspace
        Workspace_sptr ws = thisGroup[0];

        if ((m_singleGroup == int(iwp)) || m_singleGroup < 0) {
          // Either: this is the single group
          // OR: all inputs are groups
          // ... so get then entry^th workspace in this group
          if (entry < thisGroup.size()) {
            ws = thisGroup[entry];
          } else {
            // This can happen when one has more than one input group
            // workspaces, having different sizes. For example one workspace
            // group is the corrections which has N parts (e.g. weights for
            // polarized measurement) while the other one is the actual input
            // workspace group, where each item needs to be corrected together
            // with all N inputs of the second group. In this case processGroup
            // needs to be overridden, which is currently not possible in
            // python.
            throw std::runtime_error("Unable to process over groups; consider passing workspaces "
                                     "one-by-one or override processGroup method of the algorithm.");
          }
        }
        // Append the names together
        if (!outputBaseName.empty())
          outputBaseName += "_";
        outputBaseName += ws->getName();

        // Set the property using the name of that workspace
        if (auto *prop = dynamic_cast<Property *>(m_inputWorkspaceProps[iwp])) {
          if (ws->getName().empty()) {
            alg->setProperty(prop->name(), ws);
          } else {
            alg->setPropertyValue(prop->name(), ws->getName());
          }
        } else {
          throw std::logic_error("Found a Workspace property which doesn't "
                                 "inherit from Property.");
        }
      } // not an empty (i.e. optional) input
    } // for each InputWorkspace property

    std::vector<std::string> outputWSNames(m_pureOutputWorkspaceProps.size());
    // ---------- Set all the output workspaces ----------------------------
    for (size_t owp = 0; owp < m_pureOutputWorkspaceProps.size(); owp++) {
      if (auto *prop = dynamic_cast<Property *>(m_pureOutputWorkspaceProps[owp])) {
        // Default name = "in1_in2_out"
        const std::string inName = prop->value();
        if (inName.empty())
          continue;
        std::string outName;
        if (m_groupsHaveSimilarNames) {
          outName.append(inName).append("_").append(Strings::toString(entry + 1));
        } else {
          outName.append(outputBaseName).append("_").append(inName);
        }

        auto inputProp =
            std::find_if(m_inputWorkspaceProps.begin(), m_inputWorkspaceProps.end(), WorkspacePropertyValueIs(inName));

        // Overwrite workspaces in any input property if they have the same
        // name as an output (i.e. copy name button in algorithm dialog used)
        // (only need to do this for a single input, multiple will be handled
        // by ADS)
        if (inputProp != m_inputWorkspaceProps.end()) {
          const auto &inputGroup = m_unrolledInputWorkspaces[inputProp - m_inputWorkspaceProps.begin()];
          if (!inputGroup.empty())
            outName = inputGroup[entry]->getName();
        }
        // Except if all inputs had similar names, then the name is "out_1"

        // Set in the output
        alg->setPropertyValue(prop->name(), outName);

        outputWSNames[owp] = outName;
      } else {
        throw std::logic_error("Found a Workspace property which doesn't "
                               "inherit from Property.");
      }
    } // for each OutputWorkspace property

    // ------------ Execute the algo --------------
    try {
      alg->execute();
    } catch (std::exception &e) {
      std::ostringstream msg;
      msg << "Execution of " << this->name() << " for group entry " << (entry + 1) << " failed: ";
      msg << e.what(); // Add original message
      throw std::runtime_error(msg.str());
    }

    // ------------ Fill in the output workspace group ------------------
    // this has to be done after execute() because a workspace must exist
    // when it is added to a group
    for (size_t owp = 0; owp < m_pureOutputWorkspaceProps.size(); owp++) {
      auto *prop = dynamic_cast<Property *>(m_pureOutputWorkspaceProps[owp]);
      if (prop && prop->value().empty())
        continue;
      // And add it to the output group
      outGroups[owp]->add(outputWSNames[owp]);
    }

  } // for each entry in each group

  // restore group notifications
  for (auto &outGroup : outGroups) {
    outGroup->observeADSNotifications(true);
  }

  return true;
}

//--------------------------------------------------------------------------------------------
/** Copy all the non-workspace properties from this to alg
 *
 * @param alg :: other IAlgorithm
 * @param periodNum :: number of the "period" = the entry in the group + 1
 */
void Algorithm::copyNonWorkspaceProperties(IAlgorithm *alg, int periodNum) {
  if (!alg)
    throw std::runtime_error("Algorithm not created!");
  const auto &props = this->getProperties();
  for (const auto &prop : props) {
    if (prop) {

      const auto *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
      // Copy the property using the string
      if (!wsProp)
        this->setOtherProperties(alg, prop->name(), prop->value(), periodNum);
    }
  }
}

//--------------------------------------------------------------------------------------------
/** Virtual method to set the non workspace properties for this algorithm.
 * To be overridden by specific algorithms when needed.
 *
 *  @param alg :: pointer to the algorithm
 *  @param propertyName :: name of the property
 *  @param propertyValue :: value  of the property
 *  @param periodNum :: period number
 */
void Algorithm::setOtherProperties(IAlgorithm *alg, const std::string &propertyName, const std::string &propertyValue,
                                   int periodNum) {
  (void)periodNum; // Avoid compiler warning
  if (alg)
    alg->setPropertyValue(propertyName, propertyValue);
}

//--------------------------------------------------------------------------------------------
/** To query the property is a workspace property
 *  @param prop :: pointer to input properties
 *  @returns true if this is a workspace property
 */
bool Algorithm::isWorkspaceProperty(const Kernel::Property *const prop) const {
  if (!prop) {
    return false;
  }
  const auto *const wsProp = dynamic_cast<const IWorkspaceProperty *>(prop);
  return (wsProp != nullptr);
}

//=============================================================================================
//================================== Asynchronous Execution
//===================================
//=============================================================================================
namespace {
/**
 * A object to set the flag marking asynchronous running correctly
 */
struct AsyncFlagHolder {
  /** Constructor
   * @param A :: reference to the running flag
   */
  explicit AsyncFlagHolder(bool &running_flag) : m_running_flag(running_flag) { m_running_flag = true; }
  /// Destructor
  ~AsyncFlagHolder() { m_running_flag = false; }

private:
  /// Default constructor
  AsyncFlagHolder() = delete;
  /// Running flag
  bool &m_running_flag;
};
} // namespace

//--------------------------------------------------------------------------------------------
/**
 * Asynchronous execution
 */
Poco::ActiveResult<bool> Algorithm::executeAsync() {
  m_executeAsync =
      std::make_unique<Poco::ActiveMethod<bool, Poco::Void, Algorithm>>(this, &Algorithm::executeAsyncImpl);
  return (*m_executeAsync)(Poco::Void());
}

/**Callback when an algorithm is executed asynchronously
 * @param i :: Unused argument
 * @return true if executed successfully.
 */
bool Algorithm::executeAsyncImpl(const Poco::Void & /*unused*/) {
  AsyncFlagHolder running(m_runningAsync);
  return this->execute();
}

/**
 * @return A reference to the Poco::NotificationCenter object that dispatches
 * notifications
 */
Poco::NotificationCenter &Algorithm::notificationCenter() const {
  if (!m_notificationCenter)
    m_notificationCenter = std::make_unique<Poco::NotificationCenter>();
  return *m_notificationCenter;
}

/** Handles and rescales child algorithm progress notifications.
 *  @param pNf :: The progress notification from the child algorithm.
 */
void Algorithm::handleChildProgressNotification(const Poco::AutoPtr<ProgressNotification> &pNf) {
  double p = m_startChildProgress + (m_endChildProgress - m_startChildProgress) * pNf->progress;

  progress(p, pNf->message);
}

/**
 * @return A Poco:NObserver object that is responsible for reporting progress
 */
const Poco::AbstractObserver &Algorithm::progressObserver() const {
  if (!m_progressObserver)
    m_progressObserver = std::make_unique<Poco::NObserver<Algorithm, ProgressNotification>>(
        *const_cast<Algorithm *>(this), &Algorithm::handleChildProgressNotification);

  return *m_progressObserver;
}

//--------------------------------------------------------------------------------------------
/**
 * Cancel an algorithm
 */
void Algorithm::cancel() {
  // set myself to be cancelled
  m_cancel = true;

  // Loop over the output workspaces and try to cancel them
  for (auto &weakPtr : m_ChildAlgorithms) {
    if (IAlgorithm_sptr sharedPtr = weakPtr.lock()) {
      sharedPtr->cancel();
    }
  }
}

/// Returns the cancellation state
bool Algorithm::getCancel() const { return m_cancel; }

/// Returns a reference to the logger.
Kernel::Logger &Algorithm::getLogger() const { return g_log; }
/// Logging can be disabled by passing a value of false
void Algorithm::setLogging(const bool value) { g_log.setEnabled(value); }
/// returns the status of logging, True = enabled
bool Algorithm::isLogging() const { return g_log.getEnabled(); }

/* Sets the logging priority offset. Values are subtracted from the log level.
 *
 * Example value=1 will turn warning into notice
 * Example value=-1 will turn notice into warning
 */
void Algorithm::setLoggingOffset(const int value) { g_log.setLevelOffset(value); }

/// returns the logging priority offset
int Algorithm::getLoggingOffset() const { return g_log.getLevelOffset(); }

//--------------------------------------------------------------------------------------------
/** This is called during long-running operations,
 * and check if the algorithm has requested that it be cancelled.
 */
void Algorithm::interruption_point() {
  // only throw exceptions if the code is not multi threaded otherwise you
  // contravene the OpenMP standard
  // that defines that all loops must complete, and no exception can leave an
  // OpenMP section
  // openmp cancel handling is performed using the ??, ?? and ?? macros in
  // each algrothim
  IF_NOT_PARALLEL
  if (m_cancel)
    throw CancelException();
}

/**
Report that the algorithm has completed.
@param duration : Algorithm duration
@param groupProcessing : We have been processing via processGroups if true.
*/
void Algorithm::reportCompleted(const double &duration, const bool groupProcessing) {
  std::string optionalMessage;
  if (groupProcessing) {
    optionalMessage = ". Processed as a workspace group";
  }

  if (!m_isChildAlgorithm || m_alwaysStoreInADS) {
    if (m_isAlgStartupLoggingEnabled) {

      std::stringstream msg;
      msg << name() << " successful, Duration ";
      double seconds = duration;
      if (seconds > 60.) {
        auto minutes = static_cast<int>(seconds / 60.);
        msg << minutes << " minutes ";
        seconds = seconds - static_cast<double>(minutes) * 60.;
      }
      msg << std::fixed << std::setprecision(2) << seconds << " seconds" << optionalMessage;
      getLogger().notice(msg.str());
    }
  }

  else {
    getLogger().debug() << name() << " finished with isChild = " << isChild() << '\n';
  }
  setExecutionState(ExecutionState::Finished);
}

/** Registers the usage of the algorithm with the UsageService
 */
void Algorithm::registerFeatureUsage() const {
  if (UsageService::Instance().isEnabled()) {
    std::ostringstream oss;
    oss << this->name() << ".v" << this->version();
    UsageService::Instance().registerFeatureUsage(FeatureType::Algorithm, oss.str(), isChild());
  }
}

/** Enable or disable Logging of start and end messages
@param enabled : true to enable logging, false to disable
*/
void Algorithm::setAlgStartupLogging(const bool enabled) { m_isAlgStartupLoggingEnabled = enabled; }

/** return the state of logging of start and end messages
@returns : true to logging is enabled
*/
bool Algorithm::getAlgStartupLogging() const { return m_isAlgStartupLoggingEnabled; }

bool Algorithm::isCompoundProperty(const std::string &name) const {
  return std::find(m_reservedList.cbegin(), m_reservedList.cend(), name) != m_reservedList.cend();
}

//---------------------------------------------------------------------------
// Algorithm's inner classes
//---------------------------------------------------------------------------

Algorithm::AlgorithmNotification::AlgorithmNotification(const Algorithm *const alg)
    : Poco::Notification(), m_algorithm(alg) {}

const IAlgorithm *Algorithm::AlgorithmNotification::algorithm() const { return m_algorithm; }

Algorithm::StartedNotification::StartedNotification(const Algorithm *const alg) : AlgorithmNotification(alg) {}
std::string Algorithm::StartedNotification::name() const { return "StartedNotification"; } ///< class name

Algorithm::FinishedNotification::FinishedNotification(const Algorithm *const alg, bool res)
    : AlgorithmNotification(alg), success(res) {}
std::string Algorithm::FinishedNotification::name() const { return "FinishedNotification"; }

Algorithm::ProgressNotification::ProgressNotification(const Algorithm *const alg, double p, std::string msg,
                                                      double estimatedTime, int progressPrecision)
    : AlgorithmNotification(alg), progress(p), message(std::move(msg)), estimatedTime(estimatedTime),
      progressPrecision(progressPrecision) {}

std::string Algorithm::ProgressNotification::name() const { return "ProgressNotification"; }

Algorithm::ErrorNotification::ErrorNotification(const Algorithm *const alg, std::string str)
    : AlgorithmNotification(alg), what(std::move(str)) {}

std::string Algorithm::ErrorNotification::name() const { return "ErrorNotification"; }

const char *Algorithm::CancelException::what() const noexcept { return "Algorithm terminated"; }

/** Add a property to the list of managed properties
 *  @param p :: The property object to add
 *  @param doc :: A description of the property that may be displayed to users
 *  @throw Exception::ExistsError if a property with the given name already
 * exists
 */
void Algorithm::declareProperty(std::unique_ptr<Property> p, const std::string &doc) {
  m_properties.declareProperty(std::move(p), doc);
}

/** Add or replace property in the list of managed properties
 *  @param p :: The property object to add
 *  @param doc :: A description of the property that may be displayed to users
 */
void Algorithm::declareOrReplaceProperty(std::unique_ptr<Property> p, const std::string &doc) {
  m_properties.declareOrReplaceProperty(std::move(p), doc);
}

/** Reset property values back to initial values (blank or default values)
 */
void Algorithm::resetProperties() { m_properties.resetProperties(); }

/** Set the ordered list of properties by one string of values, separated by
 *semicolons.
 *
 * The string should be a json formatted collection of name value pairs
 *
 *  @param propertiesJson :: The string of property values
 *  @param ignoreProperties :: A set of names of any properties NOT to set
 *      from the propertiesArray
 *  @param createMissing :: If the property does not exist then create it
 *  @throw invalid_argument if error in parameters
 */
void Algorithm::setProperties(const std::string &propertiesJson,
                              const std::unordered_set<std::string> &ignoreProperties, bool createMissing) {
  m_properties.setProperties(propertiesJson, ignoreProperties, createMissing);
}

/** Sets all the declared properties from a json object
  @param jsonValue :: A json name value pair collection
  @param ignoreProperties :: A set of names of any properties NOT to set
  from the propertiesArray
  @param createMissing :: If the property does not exist then create it
*/
void Algorithm::setProperties(const ::Json::Value &jsonValue, const std::unordered_set<std::string> &ignoreProperties,
                              bool createMissing) {
  m_properties.setProperties(jsonValue, ignoreProperties, createMissing);
}

/** Sets all the declared properties from a string.
  @param propertiesString :: A list of name = value pairs separated by a
    semicolon
  @param ignoreProperties :: A set of names of any properties NOT to set
  from the propertiesArray
*/
void Algorithm::setPropertiesWithString(const std::string &propertiesString,
                                        const std::unordered_set<std::string> &ignoreProperties) {
  m_properties.setPropertiesWithString(propertiesString, ignoreProperties);
}

/** Set the value of a property by string
 *  N.B. bool properties must be set using 1/0 rather than true/false
 *  @param name :: The name of the property (case insensitive)
 *  @param value :: The value to assign to the property
 *  @throw Exception::NotFoundError if the named property is unknown
 *  @throw std::invalid_argument If the value is not valid for the property
 * given
 */
void Algorithm::setPropertyValue(const std::string &name, const std::string &value) {
  m_properties.setPropertyValue(name, value);
  this->afterPropertySet(name);
}

/** Set the value of a property by Json::Value object
 *  @param name :: The name of the property (case insensitive)
 *  @param value :: The value to assign to the property
 *  @throw Exception::NotFoundError if the named property is unknown
 *  @throw std::invalid_argument If the value is not valid for the property
 * given
 */
void Algorithm::setPropertyValueFromJson(const std::string &name, const Json::Value &value) {
  m_properties.setPropertyValueFromJson(name, value);
  this->afterPropertySet(name);
}

/** Set the value of a property by an index
 *  N.B. bool properties must be set using 1/0 rather than true/false
 *  @param index :: The index of the property to assign
 *  @param value :: The value to assign to the property
 *  @throw std::runtime_error if the property index is too high
 */
void Algorithm::setPropertyOrdinal(const int &index, const std::string &value) {
  m_properties.setPropertyOrdinal(index, value);
  this->afterPropertySet(m_properties.getPointerToPropertyOrdinal(index)->name());
}

/** Checks whether the named property is already in the list of managed
 * property.
 *  @param name :: The name of the property (case insensitive)
 *  @return True if the property is already stored
 */
bool Algorithm::existsProperty(const std::string &name) const { return m_properties.existsProperty(name); }

/** Validates all the properties in the collection
 *  @return True if all properties have a valid value
 */
bool Algorithm::validateProperties() const { return m_properties.validateProperties(); }

/**
 * Count the number of properties under management
 * @returns The number of properties being managed
 */
size_t Algorithm::propertyCount() const { return m_properties.propertyCount(); }

/** Get the value of a property as a string
 *  @param name :: The name of the property (case insensitive)
 *  @return The value of the named property
 *  @throw Exception::NotFoundError if the named property is unknown
 */
std::string Algorithm::getPropertyValue(const std::string &name) const { return m_properties.getPropertyValue(name); }

/** Get a property by name
 *  @param name :: The name of the property (case insensitive)
 *  @return A pointer to the named property
 *  @throw Exception::NotFoundError if the named property is unknown
 */
Property *Algorithm::getPointerToProperty(const std::string &name) const {
  return m_properties.getPointerToProperty(name);
}

/** Get a property by an index
 *  @param index :: The name of the property (case insensitive)
 *  @return A pointer to the named property
 *  @throw std::runtime_error if the property index is too high
 */
Property *Algorithm::getPointerToPropertyOrdinal(const int &index) const {
  return m_properties.getPointerToPropertyOrdinal(index);
}

/** Get the list of managed properties.
 *  The properties will be stored in the order that they were declared.
 *  @return A vector holding pointers to the list of properties
 */
const std::vector<Property *> &Algorithm::getProperties() const { return m_properties.getProperties(); }

/**
 * Return the list of declared property names.
 * @return A vector holding strings of property names
 */
std::vector<std::string> Algorithm::getDeclaredPropertyNames() const noexcept {
  return m_properties.getDeclaredPropertyNames();
}

/** Get the value of a property. Allows you to assign directly to a variable of
 *the property's type
 *  (if a supported type).
 *
 *  *** This method does NOT work for assigning to an existing std::string.
 *      In this case you have to use getPropertyValue() instead.
 *      Note that you can, though, construct a local string variable by writing,
 *      e.g. std::string s = getProperty("myProperty"). ***
 *
 *  @param name :: The name of the property
 *  @return The value of the property. Will be cast to the desired type (if a
 *supported type).
 *  @throw std::runtime_error If an attempt is made to assign a property to a
 *different type
 *  @throw Exception::NotFoundError If the property requested does not exist
 */
IPropertyManager::TypedValue Algorithm::getProperty(const std::string &name) const {
  return m_properties.getProperty(name);
}

/**
 * @param name
 * @return True if the property is its default value.
 */
bool Algorithm::isDefault(const std::string &name) const {
  return m_properties.getPointerToProperty(name)->isDefault();
}

/**
 * Return the property manager serialized as a string.
 * The format is propName=value,propName=value,propName=value
 * @param withDefaultValues :: If true then the value of default parameters will
 * be included
 * @returns A stringized version of the manager
 */
std::string Algorithm::asString(bool withDefaultValues) const { return m_properties.asString(withDefaultValues); }
/**
 * Return the property manager serialized as a json object.
 * @param withDefaultValues :: If true then the value of default parameters will
 * be included
 * @returns A jsonValue of the manager
 */
::Json::Value Algorithm::asJson(bool withDefaultValues) const { return m_properties.asJson(withDefaultValues); }

/**
 * Removes the property from properties map.
 * @param name :: Name of the property to be removed.
 *  @param delproperty :: if true, delete the named property
 */
void Algorithm::removeProperty(const std::string &name, const bool delproperty) {
  m_properties.removeProperty(name, delproperty);
}

/**
 * Removes a property from the properties map by index and return a pointer to it
 * @param index :: index of the property to be removed
 * @returns :: pointer to the removed property if found, NULL otherwise
 */
std::unique_ptr<Kernel::Property> Algorithm::takeProperty(const size_t index) {
  return m_properties.takeProperty(index);
}

/**
 * Clears all properties under management
 */
void Algorithm::clear() { m_properties.clear(); }

/**
 * Override this method to perform a custom action right after a property was
 * set.
 * The argument is the property name. Default - do nothing.
 * @param name :: A property name.
 */
void Algorithm::afterPropertySet(const std::string &name) { m_properties.afterPropertySet(name); }

} // namespace API

//---------------------------------------------------------------------------
// Specialized templated PropertyManager getValue definitions for Algorithm
// types
//---------------------------------------------------------------------------
namespace Kernel {
/**
 * Get the value of a given property as the declared concrete type
 * @param name :: The name of the property
 * @returns A pointer to an algorithm
 */
template <>
MANTID_API_DLL API::IAlgorithm_sptr IPropertyManager::getValue<API::IAlgorithm_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<API::IAlgorithm_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<IAlgorithm>";
    throw std::runtime_error(message);
  }
}

/**
 * Get the value of a given property as the declared concrete type (const
 * version)
 * @param name :: The name of the property
 * @returns A pointer to an algorithm
 */
template <>
MANTID_API_DLL API::IAlgorithm_const_sptr
IPropertyManager::getValue<API::IAlgorithm_const_sptr>(const std::string &name) const {
  const auto *prop = dynamic_cast<PropertyWithValue<API::IAlgorithm_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<IAlgorithm>";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel

} // namespace Mantid
