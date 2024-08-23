// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/ConfigService.h"
#include <thread>

namespace Mantid::API {
namespace {
/// static logger
Kernel::Logger g_log("AlgorithmManager");
} // namespace

/// Private Constructor for singleton class
AlgorithmManagerImpl::AlgorithmManagerImpl() : m_managed_algs() { g_log.debug() << "Algorithm Manager created.\n"; }

/** Private destructor
 *  Prevents client from calling 'delete' on the pointer handed
 *  out by Instance
 */
AlgorithmManagerImpl::~AlgorithmManagerImpl() = default;

/** Creates an instance of an algorithm, but does not own that instance
 *
 *  @param  algName The name of the algorithm required
 *  @param  version The version of the algorithm required, if not defined most
 *recent version is used -> version =-1
 *  @return A pointer to the created algorithm
 *  @throw  NotFoundError Thrown if algorithm requested is not registered
 */
Algorithm_sptr AlgorithmManagerImpl::createUnmanaged(const std::string &algName, const int &version) const {
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
 * recent version is used -> version =-1
 * @return A pointer to the created algorithm
 * @throw  NotFoundError Thrown if algorithm requested is not registered
 * @throw  std::runtime_error Thrown if properties string is ill-formed
 */
IAlgorithm_sptr AlgorithmManagerImpl::create(const std::string &algName, const int &version) {
  std::lock_guard<std::recursive_mutex> _lock(this->m_managedMutex);
  IAlgorithm_sptr alg;
  try {
    alg = AlgorithmFactory::Instance().create(algName,
                                              version); // Throws on fail:
    auto count = removeFinishedAlgorithms();
    g_log.debug() << count << " Finished algorithms removed from the managed algorithms list. " << m_managed_algs.size()
                  << " remaining.\n";

    // Add to list of managed ones
    m_managed_algs.emplace_back(alg);
    alg->initialize();
  } catch (std::runtime_error &ex) {
    g_log.error() << "AlgorithmManager:: Unable to create algorithm " << algName << ' ' << ex.what() << '\n';
    throw std::runtime_error("AlgorithmManager:: Unable to create algorithm " + algName + ' ' + ex.what());
  }
  return alg;
}

/**
 * Clears all managed algorithm objects that are not currently running.
 */
void AlgorithmManagerImpl::clear() {
  std::lock_guard<std::recursive_mutex> _lock(this->m_managedMutex);
  for (auto itAlg = m_managed_algs.begin(); itAlg != m_managed_algs.end();) {
    if (!(*itAlg)->isRunning()) {
      itAlg = m_managed_algs.erase(itAlg);
    } else {
      ++itAlg;
    }
  }
}

std::size_t AlgorithmManagerImpl::size() const { return m_managed_algs.size(); }

/**
 * Returns a shared pointer by algorithm id
 * @param id :: The ID of the algorithm
 * @returns A shared pointer to the algorithm or nullptr if not found
 */
IAlgorithm_sptr AlgorithmManagerImpl::getAlgorithm(AlgorithmID id) const {
  std::lock_guard<std::recursive_mutex> _lock(this->m_managedMutex);
  const auto found = std::find_if(m_managed_algs.cbegin(), m_managed_algs.cend(),
                                  [id](const auto &algorithm) { return algorithm->getAlgorithmID() == id; });
  if (found == m_managed_algs.cend()) {
    return IAlgorithm_sptr();
  } else {
    return *found;
  }
}

/**
 * Removes the given algorithm from the managed list
 * @param id :: The ID of the algorithm
 */
void AlgorithmManagerImpl::removeById(AlgorithmID id) {
  std::lock_guard<std::recursive_mutex> _lock(this->m_managedMutex);
  const auto it = std::find_if(m_managed_algs.cbegin(), m_managed_algs.cend(),
                               [&id](const auto &alg) { return alg->getAlgorithmID() == id; });
  if (it == m_managed_algs.cend()) {
    return;
  }
  if (!(*it)->isRunning()) {
    g_log.debug() << "Removing algorithm " << (*it)->name() << '\n';
    m_managed_algs.erase(it);
  } else {
    g_log.debug() << "Unable to remove algorithm " << (*it)->name() << ". The algorithm is running.\n";
  }
}

/** Called by an algorithm that is executing asynchronously
 * This sends out the notification.
 *
 * @param id :: ID of the algorithm being started
 */
void AlgorithmManagerImpl::notifyAlgorithmStarting(AlgorithmID id) {
  auto alg = this->getAlgorithm(id);
  if (!alg)
    return;
  notificationCenter.postNotification(new AlgorithmStartingNotification(alg));
}

/// Returns all running (& managed) occurances of the named algorithm, oldest
/// first
std::vector<IAlgorithm_const_sptr> AlgorithmManagerImpl::runningInstancesOf(const std::string &algorithmName) const {
  std::vector<IAlgorithm_const_sptr> theRunningInstances;
  std::lock_guard<std::recursive_mutex> _lock(this->m_managedMutex);
  std::copy_if(
      m_managed_algs.cbegin(), m_managed_algs.cend(), std::back_inserter(theRunningInstances),
      [&algorithmName](const auto &algorithm) { return algorithm->name() == algorithmName && algorithm->isRunning(); });
  return theRunningInstances;
}

/// Returns all running (& managed) occurances of any algorithm, oldest
/// first
std::vector<IAlgorithm_const_sptr> AlgorithmManagerImpl::runningInstances() const {
  std::vector<IAlgorithm_const_sptr> theRunningInstances;
  std::lock_guard<std::recursive_mutex> _lock(this->m_managedMutex);
  std::copy_if(m_managed_algs.cbegin(), m_managed_algs.cend(), std::back_inserter(theRunningInstances),
               [](const auto &algorithm) { return algorithm->isRunning(); });
  return theRunningInstances;
}

/// Requests cancellation of all running algorithms
void AlgorithmManagerImpl::cancelAll() {
  std::lock_guard<std::recursive_mutex> _lock(this->m_managedMutex);
  for (auto &managed_alg : m_managed_algs) {
    if (managed_alg->isRunning())
      managed_alg->cancel();
  }
}

/// Removes all of the finished algorithms
size_t AlgorithmManagerImpl::removeFinishedAlgorithms() {
  std::vector<IAlgorithm_const_sptr> theCompletedInstances;
  std::lock_guard<std::recursive_mutex> _lock(this->m_managedMutex);
  std::copy_if(m_managed_algs.cbegin(), m_managed_algs.cend(), std::back_inserter(theCompletedInstances),
               [](const auto &algorithm) { return (algorithm->isReadyForGarbageCollection()); });
  for (auto completedAlg : theCompletedInstances) {
    const auto it = std::find_if(m_managed_algs.begin(), m_managed_algs.end(), [&completedAlg](const auto &alg) {
      return alg->getAlgorithmID() == completedAlg->getAlgorithmID();
    });
    if (it != m_managed_algs.end()) {
      m_managed_algs.erase(it);
    }
  }
  return theCompletedInstances.size();
}

void AlgorithmManagerImpl::shutdown() {
  cancelAll();
  while (runningInstances().size() > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  clear();
}
} // namespace Mantid::API
