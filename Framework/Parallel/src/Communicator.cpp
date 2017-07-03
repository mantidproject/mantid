#include "MantidParallel/Communicator.h"

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/environment.hpp>
#endif

namespace Mantid {
namespace Parallel {

#ifdef MPI_EXPERIMENTAL
boost::mpi::environment environment;
#endif

#ifdef MPI_EXPERIMENTAL
Communicator::Communicator(const boost::mpi::communicator &comm)
    : m_communicator(comm) {}
#endif

Communicator::Communicator(boost::shared_ptr<detail::ThreadingBackend> backend,
                           const int rank)
    : m_backend(backend), m_rank(rank) {}

int Communicator::rank() const {
  if (m_backend)
    return m_rank;
#ifdef MPI_EXPERIMENTAL
  return m_communicator.rank();
#endif
  return 0;
}

int Communicator::size() const {
  if (m_backend)
    return m_backend->size();
#ifdef MPI_EXPERIMENTAL
  return m_communicator.size();
#endif
  return 1;
}

#ifdef MPI_EXPERIMENTAL
/// For internal use only. Casts the Communicator to the underlying
/// boost::mpi::communicator object.
Communicator::operator const boost::mpi::communicator &() const {
  return m_communicator;
}
#endif

/// For internal use only. Returns true if the communicator has a
/// ThreadingBackend.
bool Communicator::hasBackend() const { return static_cast<bool>(m_backend); }

/// For internal use only. Returns the ThreadingBackend or throws an exception
/// if not available.
detail::ThreadingBackend &Communicator::backend() const {
  if (!m_backend)
#ifndef MPI_EXPERIMENTAL
    throw std::runtime_error(
        "Parallel::Communicator without backend in non-MPI build.");
#else
    throw std::runtime_error("Parallel::Communicator without backend in MPI "
                             "build. Check hasBackend() before accessing the "
                             "backend.");
#endif
  return *m_backend;
}

} // namespace Parallel
} // namespace Mantid
