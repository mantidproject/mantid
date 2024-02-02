// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include <Poco/NotificationCenter.h>

#include <deque>
#include <mutex>
#include <string>
#include <utility>

namespace Mantid {
namespace API {

//----------------------------------------------------------------------------
/// Class for when an algorithm is starting asynchronously
class AlgorithmStartingNotification : public Poco::Notification {
public:
  AlgorithmStartingNotification(IAlgorithm_sptr alg) : Poco::Notification(), m_alg(std::move(alg)) {}
  /// Returns the algorithm that is starting
  IAlgorithm_sptr getAlgorithm() const { return m_alg; }

private:
  IAlgorithm_sptr m_alg; ///< Algorithm
};

//----------------------------------------------------------------------------
/** The AlgorithmManagerImpl class is responsible for controlling algorithm
    instances. It incorporates the algorithm factory and initializes algorithms.
 */
class MANTID_API_DLL AlgorithmManagerImpl {
public:
  /// Creates a managed algorithm with the option of choosing a version
  IAlgorithm_sptr create(const std::string &algName, const int &version = -1);
  /// Creates an unmanaged algorithm with the option of choosing a version
  std::shared_ptr<Algorithm> createUnmanaged(const std::string &algName, const int &version = -1) const;

  std::size_t size() const;

  IAlgorithm_sptr getAlgorithm(AlgorithmID id) const;
  void removeById(AlgorithmID id);

  std::vector<IAlgorithm_const_sptr> runningInstances() const;
  std::vector<IAlgorithm_const_sptr> runningInstancesOf(const std::string &algorithmName) const;

  /// Sends notifications to observers. Observers can subscribe to
  /// notificationCenter
  /// using Poco::NotificationCenter::addObserver(...)
  Poco::NotificationCenter notificationCenter;
  void notifyAlgorithmStarting(AlgorithmID id);

  void clear();
  void cancelAll();
  void shutdown();

private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgorithmManagerImpl>;

  AlgorithmManagerImpl();
  ~AlgorithmManagerImpl();

  /// Unimplemented copy constructor
  AlgorithmManagerImpl(const AlgorithmManagerImpl &);
  /// Unimplemented assignment operator
  AlgorithmManagerImpl &operator=(const AlgorithmManagerImpl &);

  /// Removes any finished algorithms from the list of managed algorithms
  size_t removeFinishedAlgorithms();

  /// The list of managed algorithms
  std::deque<IAlgorithm_sptr> m_managed_algs; ///<  pointers to managed algorithms [policy???]
  /// Mutex for modifying/accessing the m_managed_algs member.
  mutable std::recursive_mutex m_managedMutex;
};

using AlgorithmManager = Mantid::Kernel::SingletonHolder<AlgorithmManagerImpl>;

} // namespace API
} // Namespace Mantid

namespace Mantid {
namespace Kernel {
EXTERN_MANTID_API template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<Mantid::API::AlgorithmManagerImpl>;
}
} // namespace Mantid
