//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace API
{

// Get a reference to the logger
Kernel::Logger& AlgorithmProxy::g_log = Kernel::Logger::get("AlgorithmProxy");

//----------------------------------------------------------------------
// Public methods
//----------------------------------------------------------------------

/// Constructor
AlgorithmProxy::AlgorithmProxy(Algorithm_sptr alg) :
  PropertyManagerOwner(),_executeAsync(this,&AlgorithmProxy::executeAsyncImpl),
  m_name(alg->name()),m_category(alg->category()),
  m_version(alg->version()), m_alg(alg),
  m_isExecuted(),m_isLoggingEnabled(true), m_rethrow(false)
{
  if (!alg)
  {
    g_log.error("Unable to create a proxy algorithm.");
    throw std::logic_error("Unable to create a proxy algorithm.");
  }
  alg->initialize();
  m_OptionalMessage = alg->getOptionalMessage();
  copyPropertiesFrom(*alg);
}

/// Virtual destructor
AlgorithmProxy::~AlgorithmProxy()
{
}

/** Initialization method invoked by the framework.
 *  Does nothing for AlgorithmProxy as initialization is done in the constructor.
 */
void AlgorithmProxy::initialize()
{
  return;
}

/** The actions to be performed by the AlgorithmProxy on a dataset. This method is
 *  invoked for top level AlgorithmProxys by the application manager.
 *  This method invokes exec() method.
 *  For sub-AlgorithmProxys either the execute() method or exec() method
 *  must be EXPLICITLY invoked by  the parent AlgorithmProxy.
 *
 *  @throw runtime_error Thrown if AlgorithmProxy or sub-AlgorithmProxy cannot be executed
 */
bool AlgorithmProxy::execute()
{
  createConcreteAlg();
  try
  {
    m_alg->execute();
  }
  catch(...)
  {
    m_alg.reset();
    throw;
  }
  stopped();

  return m_isExecuted;
}

/// Asynchronous execution of the algorithm.
Poco::ActiveResult<bool> AlgorithmProxy::executeAsync()
{
  return _executeAsync(Poco::Void()); 
}

/// True if the algorithm is running asynchronously.
bool AlgorithmProxy::isRunningAsync()
{
  return m_alg ? m_alg->isRunningAsync() : false;
}

/// True if the algorithm is running.
bool AlgorithmProxy::isRunning()
{
  return m_alg ? m_alg->isRunning() : false;
}

/// Has the AlgorithmProxy already been initialized
bool AlgorithmProxy::isInitialized() const
{
  return true;//!!!!!!!!!
}

/// Has the AlgorithmProxy already been executed
bool AlgorithmProxy::isExecuted() const
{
  return m_isExecuted;
}

///Cancel the execution of the algorithm
void AlgorithmProxy::cancel()const
{
  if (m_alg)
    m_alg->cancel();
}

/** Add an observer for a notification. If the real algorithm is running
 *  the observer is added directly. If the algorithm is not running yet
 *  the observer's address is added to a buffer to be used later when execute/executeAsync
 *  method is called.
 *  @param observer :: Observer
 */
void AlgorithmProxy::addObserver(const Poco::AbstractObserver& observer)const
{
  const Poco::AbstractObserver* obs = &observer;
  if (m_alg) 
  {
    m_alg->addObserver(*obs);
  }
  // save the observer in any case because m_alg can be reset (eg in createConcreteAlg())
  m_externalObservers.push_back(obs);
}

/** Remove an observer.
 *  @param observer :: Observer
 */
void AlgorithmProxy::removeObserver(const Poco::AbstractObserver& observer)const
{
  std::vector<const Poco::AbstractObserver*>::iterator o = 
    std::find(m_externalObservers.begin(),m_externalObservers.end(),&observer);
  if (o != m_externalObservers.end()) m_externalObservers.erase(o);
  if (m_alg) m_alg->removeObserver(observer);
}

void AlgorithmProxy::setRethrows(const bool rethrow)
{
  this->m_rethrow = rethrow;
  if(m_alg) m_alg->setRethrows(rethrow);
}

/**
 * Override setPropertyValue
 * @param name The name of the property
 * @param value The value of the property as a string
 */
void AlgorithmProxy::setPropertyValue(const std::string &name, const std::string &value)
{
  createConcreteAlg(true);
  m_alg->setPropertyValue(name, value);
  copyPropertiesFrom(*m_alg);
  m_alg.reset();
}


//----------------------------------------------------------------------
// Private methods
//----------------------------------------------------------------------

/** 
 * Creates an unmanaged instance of the actual algorithm and sets its properties
 * @param initOnly If true then the algorithm will only having its init step run, otherwise observers will 
 * also be added and rethrows will be true
 */
void AlgorithmProxy::createConcreteAlg(bool initOnly)
{
  m_alg = boost::dynamic_pointer_cast<Algorithm>(AlgorithmManager::Instance().createUnmanaged(name(),version()));
  m_alg->initializeFromProxy(*this);
  if( !initOnly )
  {
    m_alg->setRethrows(this->m_rethrow);
    addObservers();
  }
}

/**
 * Clean up when the real algorithm stops
 */
void AlgorithmProxy::stopped()
{
  m_isExecuted = m_alg->isExecuted();
  m_alg.reset();
}

/**
 * Add observers stored previously in m_externalObservers
 */
void AlgorithmProxy::addObservers()
{
  if (!m_alg) return;
  std::vector<const Poco::AbstractObserver*>::reverse_iterator o = m_externalObservers.rbegin();
  for(;o != m_externalObservers.rend();o++)
    m_alg->addObserver(**o);
  m_externalObservers.clear();
}

/** executeAsync() implementation. Calls execute and when it has finished  deletes the real algorithm.
 *  @param dummy :: A dummy variable
 */
bool AlgorithmProxy::executeAsyncImpl(const Poco::Void & dummy)
{
  createConcreteAlg();
  // Call Algorithm::executeAsyncImpl rather than executeAsync() because the latter
  // would spawn off another (3rd) thread which is unecessary.
  m_alg->executeAsyncImpl(dummy); // Pass through dummy argument, though not used
  stopped();

  return m_isExecuted;
}
 ///setting the child start progress
 void AlgorithmProxy::setChildStartProgress(const double startProgress)const
 {
   m_alg->setChildStartProgress(startProgress);
 }
 /// setting the child end progress
 void AlgorithmProxy::setChildEndProgress(const double endProgress)const
 {
   m_alg->setChildEndProgress(endProgress);
 }
} // namespace API
} // namespace Mantid
