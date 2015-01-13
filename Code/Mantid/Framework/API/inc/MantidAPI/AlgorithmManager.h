#ifndef MANTID_API_ALGORITHMMANAGER_H_
#define MANTID_API_ALGORITHMMANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <deque>
#include <string>
#include <Poco/NotificationCenter.h>
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace API {

//----------------------------------------------------------------------------
/// Class for when an algorithm is starting asynchronously
class AlgorithmStartingNotification : public Poco::Notification {
public:
  AlgorithmStartingNotification(IAlgorithm_sptr alg)
      : Poco::Notification(), m_alg(alg) {}
  /// Returns the algorithm that is starting
  IAlgorithm_sptr getAlgorithm() const { return m_alg; }

private:
  IAlgorithm_sptr m_alg; ///< Algorithm
};

//----------------------------------------------------------------------------
/** The AlgorithmManagerImpl class is responsible for controlling algorithm
    instances. It incorporates the algorithm factory and initializes algorithms.

    Copyright &copy; 2007-2013 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
class MANTID_API_DLL AlgorithmManagerImpl {
public:
  /// Creates a managed algorithm with the option of choosing a version
  IAlgorithm_sptr create(const std::string &algName, const int &version = -1,
                         bool makeProxy = true);
  /// Creates an unmanaged algorithm with the option of choosing a version
  boost::shared_ptr<Algorithm> createUnmanaged(const std::string &algName,
                                               const int &version = -1) const;

  std::size_t size() const;
  void setMaxAlgorithms(int n);

  IAlgorithm_sptr getAlgorithm(AlgorithmID id) const;
  void removeById(AlgorithmID id);
  IAlgorithm_sptr newestInstanceOf(const std::string &algorithmName) const;
  std::vector<IAlgorithm_const_sptr>
  runningInstancesOf(const std::string &algorithmName) const;

  /// Sends notifications to observers. Observers can subscribe to
  /// notificationCenter
  /// using Poco::NotificationCenter::addObserver(...)
  Poco::NotificationCenter notificationCenter;
  void notifyAlgorithmStarting(AlgorithmID id);

  void clear();
  void cancelAll();

private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgorithmManagerImpl>;

  AlgorithmManagerImpl();
  ~AlgorithmManagerImpl();

  /// Unimplemented copy constructor
  AlgorithmManagerImpl(const AlgorithmManagerImpl &);
  /// Unimplemented assignment operator
  AlgorithmManagerImpl &operator=(const AlgorithmManagerImpl &);

  /// The maximum size of the algorithm store
  int m_max_no_algs;
  /// The list of managed algorithms
  std::deque<IAlgorithm_sptr>
      m_managed_algs; ///<  pointers to managed algorithms [policy???]
  /// Mutex for modifying/accessing the m_managed_algs member.
  mutable Kernel::Mutex m_managedMutex;
};

/// Forward declaration of a specialisation of SingletonHolder for
/// AlgorithmManagerImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<AlgorithmManagerImpl>;
#endif /* _WIN32 */
typedef Mantid::Kernel::SingletonHolder<AlgorithmManagerImpl> AlgorithmManager;

} // namespace API
} // Namespace Mantid

#endif /* MANTID_API_ALGORITHMMANAGER_H_ */
