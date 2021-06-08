// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/Request.h"
#include "MantidParallel/Status.h"
#include "MantidParallel/ThreadingBackend.h"

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/communicator.hpp>
#endif

namespace boost {
namespace mpi {
class environment;
}
} // namespace boost

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
*/
class MANTID_PARALLEL_DLL Communicator {
public:
  Communicator() = default;
#ifdef MPI_EXPERIMENTAL
  explicit Communicator(const boost::mpi::communicator &comm);
#endif

  int rank() const;
  int size() const;
  template <typename... T> void send(T &&...args) const;
  template <typename... T> Status recv(T &&...args) const;
  template <typename... T> Request isend(T &&...args) const;
  template <typename... T> Request irecv(T &&...args) const;

#ifdef MPI_EXPERIMENTAL
  operator const boost::mpi::communicator &() const;
#endif

  bool hasBackend() const;
  detail::ThreadingBackend &backend() const;

private:
  Communicator(std::shared_ptr<detail::ThreadingBackend> backend, const int rank);

#ifdef MPI_EXPERIMENTAL
  boost::mpi::communicator m_communicator;
#endif
  std::shared_ptr<detail::ThreadingBackend> m_backend;
  int m_rank{0};

  // For accessing constructor with threading backend.
  friend class ParallelTestHelpers::ParallelRunner;
};

template <typename... T> void Communicator::send(T &&...args) const {
#ifdef MPI_EXPERIMENTAL
  if (!hasBackend())
    return m_communicator.send(std::forward<T>(args)...);
#endif
  backend().send(m_rank, std::forward<T>(args)...);
}

template <typename... T> Status Communicator::recv(T &&...args) const {
#ifdef MPI_EXPERIMENTAL
  if (!hasBackend())
    return Status(m_communicator.recv(std::forward<T>(args)...));
#endif
  return backend().recv(m_rank, std::forward<T>(args)...);
}

template <typename... T> Request Communicator::isend(T &&...args) const {
#ifdef MPI_EXPERIMENTAL
  if (!hasBackend())
    return m_communicator.isend(std::forward<T>(args)...);
#endif
  return backend().isend(m_rank, std::forward<T>(args)...);
}

template <typename... T> Request Communicator::irecv(T &&...args) const {
#ifdef MPI_EXPERIMENTAL
  if (!hasBackend())
    return m_communicator.irecv(std::forward<T>(args)...);
#endif
  return backend().irecv(m_rank, std::forward<T>(args)...);
}

} // namespace Parallel
} // namespace Mantid
