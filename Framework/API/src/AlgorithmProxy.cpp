// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidKernel/StringTokenizer.h"

#include <Poco/ActiveMethod.h>
#include <Poco/ActiveResult.h>
#include <Poco/Void.h>
#include <json/value.h>

using namespace Mantid::Kernel;

namespace Mantid {
namespace API {

/// Constructor
AlgorithmProxy::AlgorithmProxy(Algorithm_sptr alg)
    : PropertyManagerOwner(),
      m_executeAsync(new Poco::ActiveMethod<bool, Poco::Void, AlgorithmProxy>(
          this, &AlgorithmProxy::executeAsyncImpl)),
      m_name(alg->name()), m_category(alg->category()),
      m_categorySeparator(alg->categorySeparator()), m_seeAlso(alg->seeAlso()),
      m_alias(alg->alias()), m_summary(alg->summary()),
      m_version(alg->version()), m_alg(alg), m_isExecuted(),
      m_isLoggingEnabled(true), m_loggingOffset(0),
      m_isAlgStartupLoggingEnabled(true), m_rethrow(false), m_isChild(false),
      m_setAlwaysStoreInADS(true) {
  if (!alg) {
    throw std::logic_error("Unable to create a proxy algorithm.");
  }
  alg->initialize();
  copyPropertiesFrom(*alg);
}

/// Virtual destructor
AlgorithmProxy::~AlgorithmProxy() { delete m_executeAsync; }

/** Initialization method invoked by the framework.
 *  Does nothing for AlgorithmProxy as initialization is done in the
 * constructor.
 */
void AlgorithmProxy::initialize() {}

AlgorithmID AlgorithmProxy::getAlgorithmID() const {
  return AlgorithmID(const_cast<AlgorithmProxy *>(this));
}

/** Perform whole-input validation */
std::map<std::string, std::string> AlgorithmProxy::validateInputs() {
  if (!m_alg)
    createConcreteAlg(true);
  return m_alg->validateInputs();
}

/** The actions to be performed by the AlgorithmProxy on a dataset. This method
 *is
 *  invoked for top level AlgorithmProxys by the application manager.
 *  This method invokes exec() method.
 *  For Child AlgorithmProxys either the execute() method or exec() method
 *  must be EXPLICITLY invoked by  the parent AlgorithmProxy.
 *
 *  @throw runtime_error Thrown if AlgorithmProxy or Child AlgorithmProxy cannot
 *be executed
 */
bool AlgorithmProxy::execute() {
  createConcreteAlg(false);
  try {
    m_alg->execute();
  } catch (...) {
    stopped();
    throw;
  }
  stopped();
  return m_isExecuted;
}

/** Asynchronous execution of the algorithm.
 * This will launch the AlgorithmProxy::executeAsyncImpl() method
 * but in a separate thread.
 *
 * @return Poco::ActiveResult containing the result from the thread.
 */
Poco::ActiveResult<bool> AlgorithmProxy::executeAsync() {
  return (*m_executeAsync)(Poco::Void());
}

/** executeAsync() implementation.
 * Calls execute and, when it has finished, deletes the real algorithm.
 *  @param dummy :: An unused dummy variable
 */
bool AlgorithmProxy::executeAsyncImpl(const Poco::Void &dummy) {
  createConcreteAlg(false);
  // Call Algorithm::executeAsyncImpl rather than executeAsync() because the
  // latter
  // would spawn off another (3rd) thread which is unecessary.
  try {
    m_alg->executeAsyncImpl(
        dummy); // Pass through dummy argument, though not used
  } catch (...) {
    stopped();
    throw;
  }
  stopped();
  return m_isExecuted;
}

/// True if the algorithm is running.
bool AlgorithmProxy::isRunning() const {
  return m_alg ? m_alg->isRunning() : false;
}

/// Has the AlgorithmProxy already been initialized
bool AlgorithmProxy::isInitialized() const {
  return true; //!!!!!!!!!
}

/// Has the AlgorithmProxy already been executed
bool AlgorithmProxy::isExecuted() const { return m_isExecuted; }

/// Cancel the execution of the algorithm
void AlgorithmProxy::cancel() {
  if (m_alg)
    m_alg->cancel();
}

/** Add an observer for a notification. If the real algorithm is running
 *  the observer is added directly. If the algorithm is not running yet
 *  the observer's address is added to a buffer to be used later when
 * execute/executeAsync
 *  method is called.
 *  @param observer :: Observer
 */
void AlgorithmProxy::addObserver(const Poco::AbstractObserver &observer) const {
  const Poco::AbstractObserver *obs = &observer;
  if (m_alg) {
    m_alg->addObserver(*obs);
  }
  // save the observer in any case because m_alg can be reset (eg in
  // createConcreteAlg())
  m_externalObservers.push_back(obs);
}

/** Remove an observer.
 *  @param observer :: Observer
 */
void AlgorithmProxy::removeObserver(
    const Poco::AbstractObserver &observer) const {
  auto o = std::find(m_externalObservers.begin(), m_externalObservers.end(),
                     &observer);
  if (o != m_externalObservers.end())
    m_externalObservers.erase(o);
  if (m_alg)
    m_alg->removeObserver(observer);
}

void AlgorithmProxy::setRethrows(const bool rethrow) {
  this->m_rethrow = rethrow;
  if (m_alg)
    m_alg->setRethrows(rethrow);
}

/**
 * @return A string giving the method name that should be attached to a
 * workspace
 */
const std::string AlgorithmProxy::workspaceMethodName() const {
  if (m_alg)
    return m_alg->workspaceMethodName();
  else
    return "";
}

/**
 * @return A set of workspace class names that should have the
 * workspaceMethodName attached
 */
const std::vector<std::string> AlgorithmProxy::workspaceMethodOn() const {
  if (m_alg)
    return m_alg->workspaceMethodOn();
  else
    return std::vector<std::string>();
}

/**
 * @return The name of the property that the calling object will be passed to
 */
const std::string AlgorithmProxy::workspaceMethodInputProperty() const {
  if (m_alg)
    return m_alg->workspaceMethodInputProperty();
  else
    return "";
}

/**
 * Override setPropertyValue
 * @param name The name of the property
 * @param value The value of the property as a string
 */
void AlgorithmProxy::setPropertyValue(const std::string &name,
                                      const std::string &value) {
  createConcreteAlg(true);
  m_alg->setPropertyValue(name, value);
  copyPropertiesFrom(*m_alg);
}

/**
 * Do something after a property was set
 * @param name :: The name of the property
 */
void AlgorithmProxy::afterPropertySet(const std::string &name) {
  createConcreteAlg(true);
  m_alg->getPointerToProperty(name)->setValueFromProperty(
      *this->getPointerToProperty(name));
  m_alg->afterPropertySet(name);
  copyPropertiesFrom(*m_alg);
}

/**
 * Copy properties from another property manager
 * Making sure that the concrete alg is kept in sync
 * @param po :: The property manager to copy
 */
void AlgorithmProxy::copyPropertiesFrom(const PropertyManagerOwner &po) {
  PropertyManagerOwner::copyPropertiesFrom(po);
  createConcreteAlg(true);
  m_alg->copyPropertiesFrom(*this);
}

//----------------------------------------------------------------------
// Private methods
//----------------------------------------------------------------------

/**
 * Creates an unmanaged instance of the actual algorithm and sets its properties
 * @param initOnly If true then the algorithm will only having its init step
 * run, otherwise observers will also be added and rethrows will be true
 */
void AlgorithmProxy::createConcreteAlg(bool initOnly) {
  if ((m_alg) && initOnly) {
    // the cached algorithm exists and is not going to be executed,
    // use that one
  } else {
    m_alg = boost::dynamic_pointer_cast<Algorithm>(
        AlgorithmManager::Instance().createUnmanaged(name(), version()));
    m_alg->initializeFromProxy(*this);
    if (!initOnly) {
      m_alg->setRethrows(this->m_rethrow);
      addObservers();
    }
  }
}

/**
 * Clean up when the real algorithm stops
 */
void AlgorithmProxy::stopped() {
  if (m_setAlwaysStoreInADS)
    dropWorkspaceReferences();
  m_isExecuted = m_alg->isExecuted();
  m_alg.reset();
}

/**
 * Forces any workspace property to clear its internal workspace reference
 */
void AlgorithmProxy::dropWorkspaceReferences() {
  const std::vector<Property *> &props = getProperties();
  for (auto prop : props) {
    if (auto *wsProp = dynamic_cast<IWorkspaceProperty *>(prop)) {
      wsProp->clear();
    }
  }
}

/**
 * Add observers stored previously in m_externalObservers
 */
void AlgorithmProxy::addObservers() {
  if (!m_alg)
    return;
  for (auto o = m_externalObservers.rbegin(); o != m_externalObservers.rend();
       ++o)
    m_alg->addObserver(**o);
  m_externalObservers.clear();
}

/// setting the child start progress
void AlgorithmProxy::setChildStartProgress(const double startProgress) const {
  m_alg->setChildStartProgress(startProgress);
}
/// setting the child end progress
void AlgorithmProxy::setChildEndProgress(const double endProgress) const {
  m_alg->setChildEndProgress(endProgress);
}

/**
 * Serialize this object to a string. Simple routes the call the algorithm
 * @returns This object serialized as a string
 */
std::string AlgorithmProxy::toString() const {
  const_cast<AlgorithmProxy *>(this)->createConcreteAlg(true);
  return m_alg->toString();
}

/**
 * Serialize this object to a Json::Value. Simple routes the call the algorithm
 * @returns This object serialized as a Json::Value
 */
Json::Value AlgorithmProxy::toJson() const {
  const_cast<AlgorithmProxy *>(this)->createConcreteAlg(true);
  return m_alg->toJson();
}

/// Function to return all of the categories that contain this algorithm
const std::vector<std::string> AlgorithmProxy::categories() const {
  Mantid::Kernel::StringTokenizer tokenizer(
      category(), categorySeparator(),
      Mantid::Kernel::StringTokenizer::TOK_TRIM |
          Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);

  auto res = tokenizer.asVector();

  const DeprecatedAlgorithm *depo =
      dynamic_cast<const DeprecatedAlgorithm *>(this);
  if (depo != nullptr) {
    res.emplace_back("Deprecated");
  }
  return res;
}

/** Enable or disable Logging of start and end messages
@param enabled : true to enable logging, false to disable
*/
void AlgorithmProxy::setAlgStartupLogging(const bool enabled) {
  m_isAlgStartupLoggingEnabled = enabled;
}

/** return the state of logging of start and end messages
@returns : true to logging is enabled
*/
bool AlgorithmProxy::getAlgStartupLogging() const {
  return m_isAlgStartupLoggingEnabled;
}
} // namespace API
} // namespace Mantid
