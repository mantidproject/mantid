//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MultiThreaded.h"

using Mantid::Kernel::Mutex;

namespace Mantid {
namespace API {
namespace {
/// static logger
Kernel::Logger g_log("AlgorithmManager");
}

/// Private Constructor for singleton class
AlgorithmManagerImpl::AlgorithmManagerImpl() : m_managed_algs() {
  if (!Kernel::ConfigService::Instance().getValue("algorithms.retained",
                                                  m_max_no_algs) ||
      m_max_no_algs < 1) {
    m_max_no_algs = 100; // Default to keeping 100 algorithms if not specified
  }

  g_log.debug() << "Algorithm Manager created." << std::endl;
}

/** Private destructor
*  Prevents client from calling 'delete' on the pointer handed
*  out by Instance
*/
AlgorithmManagerImpl::~AlgorithmManagerImpl() {}

/** Creates an instance of an algorithm, but does not own that instance
*
*  @param  algName The name of the algorithm required
*  @param  version The version of the algorithm required, if not defined most
*recent version is used -> version =-1
*  @return A pointer to the created algorithm
*  @throw  NotFoundError Thrown if algorithm requested is not registered
*/
Algorithm_sptr AlgorithmManagerImpl::createUnmanaged(const std::string &algName,
                                                     const int &version) const {
  return AlgorithmFactory::Instance().create(algName,
                                             version); // Throws on fail:
}

/** Creates and initialises an instance of an algorithm.
 *
 * The algorithm gets tracked in the list of "managed" algorithms,
 * which is shown in GUI for cancelling, etc.
 *
 * @param  algName :: The name of the algorithm required
 * @param  version :: The version of the algorithm required, if not defined most
 *recent version is used -> version =-1
 * @param  makeProxy :: If true (default), create and return AlgorithmProxy of
 *the given algorithm.
 *         DO NOT SET TO FALSE unless you are really sure of what you are doing!
 * @return A pointer to the created algorithm
 * @throw  NotFoundError Thrown if algorithm requested is not registered
 * @throw  std::runtime_error Thrown if properties string is ill-formed
*/
IAlgorithm_sptr AlgorithmManagerImpl::create(const std::string &algName,
                                             const int &version,
                                             bool makeProxy) {
  Mutex::ScopedLock _lock(this->m_managedMutex);
  IAlgorithm_sptr alg;
  try {
    Algorithm_sptr unmanagedAlg = AlgorithmFactory::Instance().create(
        algName, version); // Throws on fail:
    if (makeProxy)
      alg = IAlgorithm_sptr(new AlgorithmProxy(unmanagedAlg));
    else
      alg = unmanagedAlg;

    // If this takes us beyond the maximum size, then remove the oldest one(s)
    while (m_managed_algs.size() >=
           static_cast<std::deque<IAlgorithm_sptr>::size_type>(m_max_no_algs)) {
      std::deque<IAlgorithm_sptr>::iterator it;
      it = m_managed_algs.begin();

      // Look for the first (oldest) algo that is NOT running right now.
      while (it != m_managed_algs.end()) {
        if (!(*it)->isRunning())
          break;
        ++it;
      }

      if (it == m_managed_algs.end()) {
        // Unusual case where ALL algorithms are running
        g_log.warning()
            << "All algorithms in the AlgorithmManager are running. "
            << "Cannot pop oldest algorithm. "
            << "You should increase your 'algorithms.retained' value. "
            << m_managed_algs.size() << " in queue." << std::endl;
        break;
      } else {
        // Normal; erase that algorithm
        g_log.debug() << "Popping out oldest algorithm " << (*it)->name()
                      << std::endl;
        m_managed_algs.erase(it);
      }
    }

    // Add to list of managed ones
    m_managed_algs.push_back(alg);
    alg->initialize();

  } catch (std::runtime_error &ex) {
    g_log.error() << "AlgorithmManager:: Unable to create algorithm " << algName
                  << ' ' << ex.what() << std::endl;
    throw std::runtime_error("AlgorithmManager:: Unable to create algorithm " +
                             algName + ' ' + ex.what());
  }
  return alg;
}

/**
 * Clears all managed algorithm objects.
 */
void AlgorithmManagerImpl::clear() {
  Mutex::ScopedLock _lock(this->m_managedMutex);
  m_managed_algs.clear();
  return;
}

std::size_t AlgorithmManagerImpl::size() const { return m_managed_algs.size(); }

/**
 * Set new maximum number of algorithms that can be stored.
 *
 * @param n :: The new maximum.
 */
void AlgorithmManagerImpl::setMaxAlgorithms(int n) {
  if (n < 0) {
    throw std::runtime_error("Maximum number of algorithms stored in "
                             "AlgorithmManager cannot be negative.");
  }
  m_max_no_algs = n;
}

/**
 * Returns a shared pointer by algorithm id
 * @param id :: The ID of the algorithm
 * @returns A shared pointer to the algorithm
 */
IAlgorithm_sptr AlgorithmManagerImpl::getAlgorithm(AlgorithmID id) const {
  Mutex::ScopedLock _lock(this->m_managedMutex);
  for (std::deque<IAlgorithm_sptr>::const_iterator a = m_managed_algs.begin();
       a != m_managed_algs.end(); ++a) {
    if ((**a).getAlgorithmID() == id)
      return *a;
  }
  return IAlgorithm_sptr();
}

/**
 * Removes the given algorithm from the managed list
 * @param id :: The ID of the algorithm
 */
void AlgorithmManagerImpl::removeById(AlgorithmID id) {
  Mutex::ScopedLock _lock(this->m_managedMutex);
  auto itend = m_managed_algs.end();
  for (auto it = m_managed_algs.begin(); it != itend; ++it) {
    if ((**it).getAlgorithmID() == id) {
      if (!(*it)->isRunning()) {
        g_log.debug() << "Removing algorithm " << (*it)->name() << std::endl;
        m_managed_algs.erase(it);
      } else {
        g_log.debug() << "Unable to remove algorithm " << (*it)->name()
                      << ". The algorithm is running." << std::endl;
      }
      break;
    }
  }
}

/** Called by an algorithm that is executing asynchronously
 * This sends out the notification.
 *
 * @param id :: ID of the algorithm being started
 */
void AlgorithmManagerImpl::notifyAlgorithmStarting(AlgorithmID id) {
  IAlgorithm_sptr alg = this->getAlgorithm(id);
  if (!alg)
    return;
  notificationCenter.postNotification(new AlgorithmStartingNotification(alg));
}

/// Returns the most recently created instance of the named algorithm (or null
/// if not found)
IAlgorithm_sptr
AlgorithmManagerImpl::newestInstanceOf(const std::string &algorithmName) const {
  for (auto it = m_managed_algs.rbegin(); it != m_managed_algs.rend(); ++it) {
    if ((*it)->name() == algorithmName)
      return *it;
  }

  return IAlgorithm_sptr();
}

/// Returns all running (& managed) occurances of the named algorithm, oldest
/// first
std::vector<IAlgorithm_const_sptr> AlgorithmManagerImpl::runningInstancesOf(
    const std::string &algorithmName) const {
  std::vector<IAlgorithm_const_sptr> theRunningInstances;
  Mutex::ScopedLock _lock(this->m_managedMutex);
  for (auto alg = m_managed_algs.begin(); alg != m_managed_algs.end(); ++alg) {
    auto currentAlgorithm = *alg;
    if (currentAlgorithm->name() == algorithmName &&
        currentAlgorithm->isRunning()) {
      theRunningInstances.push_back(currentAlgorithm);
    }
  }

  return theRunningInstances;
}

/// Requests cancellation of all running algorithms
void AlgorithmManagerImpl::cancelAll() {
  Mutex::ScopedLock _lock(this->m_managedMutex);
  for (auto it = m_managed_algs.begin(); it != m_managed_algs.end(); ++it) {
    if ((*it)->isRunning())
      (*it)->cancel();
  }
}

} // namespace API
} // namespace Mantid
