//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmObserver.h"

#include <iostream>

using namespace Mantid::Kernel;

namespace Mantid
{
namespace API
{

/**  Helper class which observes finish and error notifications from 
     the asynchronously running algorithm. It in turn notifies the
     proxy algorithm about the events.
  */
class AlgorithmProxyObserver: public AlgorithmObserver
{
    AlgorithmProxy* m_AlgProxy;
public:
    AlgorithmProxyObserver(AlgorithmProxy* ap):AlgorithmObserver(),m_AlgProxy(ap){}
    void finishHandle(const IAlgorithm* alg)
    {
        m_AlgProxy->stopped();
    }
    void errorHandle(const IAlgorithm* alg)
    {
        m_AlgProxy->stopped();
    }
};

// Get a reference to the logger
Kernel::Logger& AlgorithmProxy::g_log = Kernel::Logger::get("AlgorithmProxyProxy");

/****************************************************************
 Public methods
/****************************************************************/

/// Constructor
AlgorithmProxy::AlgorithmProxy(IAlgorithm_sptr alg) :
  PropertyManagerOwner(),m_name(alg->name()),m_category(alg->category()),m_version(alg->version()),m_isExecuted()
      ,m_observer(new AlgorithmProxyObserver(this))
{
    Algorithm_sptr a = boost::dynamic_pointer_cast<Algorithm>(alg);
    if (!a)
    {
        g_log.error("Unable to create a proxy algorithm.");
        throw std::logic_error("Unable to create a proxy algorithm.");
    }
    a->initialize();
    copyPropertiesFrom(*a);
}

/// Virtual destructor
AlgorithmProxy::~AlgorithmProxy()
{
    delete m_observer;
}

/** Initialization method invoked by the framework. This method is responsible
 *  for any bookkeeping of initialization required by the framework itself.
 *  It will in turn invoke the init() method of the derived AlgorithmProxy,
 *  and of any sub-AlgorithmProxys which it creates.
 *  @throw runtime_error Thrown if AlgorithmProxy or sub-AlgorithmProxy cannot be initialised
 * 
 */
void AlgorithmProxy::initialize()
{
  // Do nothing as initialization is done in the constructor
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
    m_alg = boost::dynamic_pointer_cast<Algorithm>(AlgorithmManager::Instance().createUnmanaged(name(),version()));
    m_alg->initializeFromProxy(*this);
    addObservers();
    try
    {
        m_alg->execute();
    }
    catch(...)
    {
        // zero the pointer and rethrow
        m_alg.reset();
        throw;
    }
    m_isExecuted = m_alg->isExecuted();
    m_alg.reset();
    return m_isExecuted;
}

/// Execute asynchronously
Poco::ActiveResult<bool> AlgorithmProxy::executeAsync()
{
    m_alg = boost::dynamic_pointer_cast<Algorithm>(AlgorithmManager::Instance().createUnmanaged(name(),version()));
    m_alg->initializeFromProxy(*this);
    addObservers();
    m_observer->observeFinish(m_alg);
    m_observer->observeError(m_alg);
    return m_alg->executeAsync();
}

/// True if the algorithm is running asynchronously.
bool AlgorithmProxy::isRunningAsync()
{
    return m_alg? m_alg->isRunningAsync():false;
}

/// True if the algorithm is running.
bool AlgorithmProxy::isRunning()
{
    return m_alg? m_alg->isRunning():false;
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
void AlgorithmProxy::cancel()const
{
    if (m_alg)
        m_alg->cancel();
}

/** Add an observer for a notification. If the real algorithm is running
    the observer is added directly sraight away. If the algorithm is not running yet
    the observers address is added to a buffer to be used later when execute/executeAsync
    method is called.
 */
void AlgorithmProxy::addObserver(const Poco::AbstractObserver& observer)const
{
    const Poco::AbstractObserver* obs = &observer;
    if (m_alg) 
    {
        m_alg->addObserver(*obs);
    }
    else
        m_externalObservers.push_back(obs);
}

/// Remove an observer
void AlgorithmProxy::removeObserver(const Poco::AbstractObserver& observer)const
{
    std::vector<const Poco::AbstractObserver*>::iterator o = 
        std::find(m_externalObservers.begin(),m_externalObservers.end(),&observer);
    if (o != m_externalObservers.end()) m_externalObservers.erase(o);
    if (m_alg) m_alg->removeObserver(observer);
}

/****************************************************************
 Private methods
/****************************************************************/

void AlgorithmProxy::stopped()
{
    m_isExecuted = m_alg->isExecuted();
    //m_alg.reset();
}

void AlgorithmProxy::addObservers()
{
    if (!m_alg) return;
    std::vector<const Poco::AbstractObserver*>::reverse_iterator o = m_externalObservers.rbegin();
    for(;o != m_externalObservers.rend();o++)
        m_alg->addObserver(**o);
    m_externalObservers.clear();
}

} // namespace API
} // namespace Mantid
