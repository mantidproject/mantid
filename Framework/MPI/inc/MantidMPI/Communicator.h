#ifndef MANTID_MPI_COMMUNICATOR_H_
#define MANTID_MPI_COMMUNICATOR_H_

#include "MantidMPI/DllConfig.h"
#include "MantidMPI/Request.h"
#include "MantidMPI/ThreadingBackend.h"

#include <boost/make_shared.hpp>
#include <boost/mpi/communicator.hpp>

namespace Mantid {
namespace MPI {
class ParallelRunner;

/** Wrapper for boost::mpi::communicator. For non-MPI builds an equivalent
  implementation is provided.

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
class MANTID_MPI_DLL Communicator {
public:
  Communicator() = default;
  explicit Communicator(const boost::mpi::communicator &comm);

  int rank() const;
  int size() const;
  template <typename... T> void send(T &&... args) const;
  template <typename... T> void recv(T &&... args) const;
  template <typename... T> Request isend(T &&... args) const;
  template <typename... T> Request irecv(T &&... args) const;

private:
  Communicator(boost::shared_ptr<detail::ThreadingBackend> backend,
               const int rank);

  boost::mpi::communicator m_communicator;
  boost::shared_ptr<detail::ThreadingBackend> m_backend;
  const int m_rank{0};

  // For accessing constructor with threading backend.
  friend class ParallelRunner;
};

template <typename... T> void Communicator::send(T &&... args) const {
  if (m_backend)
    return m_backend->send(m_rank, std::forward<T>(args)...);
  m_communicator.send(std::forward<T>(args)...);
}

template <typename... T> void Communicator::recv(T &&... args) const {
  // Not returning a status since it would usually not get initialized. See
  // http://mpi-forum.org/docs/mpi-1.1/mpi-11-html/node35.html#Node35.
  if (m_backend)
    return m_backend->recv(m_rank, std::forward<T>(args)...);
  static_cast<void>(m_communicator.recv(std::forward<T>(args)...));
}

template <typename... T> Request Communicator::isend(T &&... args) const {
  if (m_backend)
    return m_backend->isend(m_rank, std::forward<T>(args)...);
  return m_communicator.isend(std::forward<T>(args)...);
}

template <typename... T> Request Communicator::irecv(T &&... args) const {
  if (m_backend)
    return m_backend->irecv(m_rank, std::forward<T>(args)...);
  return m_communicator.irecv(std::forward<T>(args)...);
}

} // namespace MPI
} // namespace Mantid

#endif /* MANTID_MPI_COMMUNICATOR_H_ */
