#ifndef MANTID_PARALLEL_COMMUNICATOR_H_
#define MANTID_PARALLEL_COMMUNICATOR_H_

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/Request.h"
#include "MantidParallel/ThreadingBackend.h"

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/communicator.hpp>
#endif

namespace boost {
namespace mpi {
class environment;
}
}

namespace ParallelTestHelpers {
class ParallelRunner;
}

namespace Mantid {
namespace Parallel {
#ifdef MPI_EXPERIMENTAL
extern boost::mpi::environment environment;
#endif

/** Wrapper for boost::mpi::communicator. For non-MPI builds an equivalent
  implementation with reduced functionality is provided.

  @author Simon Heybrock
  @date 2017

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_PARALLEL_DLL Communicator {
public:
  Communicator() = default;
#ifdef MPI_EXPERIMENTAL
  explicit Communicator(const boost::mpi::communicator &comm);
#endif

  int rank() const;
  int size() const;
  template <typename... T> void send(T &&... args) const;
  template <typename... T> void recv(T &&... args) const;
  template <typename... T> Request isend(T &&... args) const;
  template <typename... T> Request irecv(T &&... args) const;

#ifdef MPI_EXPERIMENTAL
  operator const boost::mpi::communicator &() const;
#endif

  bool hasBackend() const;
  detail::ThreadingBackend &backend() const;

private:
  Communicator(boost::shared_ptr<detail::ThreadingBackend> backend,
               const int rank);

#ifdef MPI_EXPERIMENTAL
  boost::mpi::communicator m_communicator;
#endif
  boost::shared_ptr<detail::ThreadingBackend> m_backend;
  int m_rank{0};

  // For accessing constructor with threading backend.
  friend class ParallelTestHelpers::ParallelRunner;
};

template <typename... T> void Communicator::send(T &&... args) const {
#ifdef MPI_EXPERIMENTAL
  if (!hasBackend())
    return m_communicator.send(std::forward<T>(args)...);
#endif
  backend().send(m_rank, std::forward<T>(args)...);
}

template <typename... T> void Communicator::recv(T &&... args) const {
// Not returning a status since it would usually not get initialized. See
// http://mpi-forum.org/docs/mpi-1.1/mpi-11-html/node35.html#Node35.
#ifdef MPI_EXPERIMENTAL
  if (!hasBackend())
    return static_cast<void>(m_communicator.recv(std::forward<T>(args)...));
#endif
  backend().recv(m_rank, std::forward<T>(args)...);
}

template <typename... T> Request Communicator::isend(T &&... args) const {
#ifdef MPI_EXPERIMENTAL
  if (!hasBackend())
    return m_communicator.isend(std::forward<T>(args)...);
#endif
  return backend().isend(m_rank, std::forward<T>(args)...);
}

template <typename... T> Request Communicator::irecv(T &&... args) const {
#ifdef MPI_EXPERIMENTAL
  if (!hasBackend())
    return m_communicator.irecv(std::forward<T>(args)...);
#endif
  return backend().irecv(m_rank, std::forward<T>(args)...);
}

} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_COMMUNICATOR_H_ */
