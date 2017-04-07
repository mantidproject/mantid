#include "MantidParallel/Communicator.h"

namespace Mantid {
namespace Parallel {

#ifdef MPI_EXPERIMENTAL
Communicator::Communicator(const boost::mpi::communicator &comm)
    : m_communicator(comm) {}

Communicator::Communicator(boost::shared_ptr<detail::ThreadingBackend> backend,
                           const int rank)
    : m_backend(backend), m_rank(rank) {}
#endif

int Communicator::rank() const {
#ifndef MPI_EXPERIMENTAL
  return 0;
#else
  if (m_backend)
    return m_rank;
  return m_communicator.rank();
#endif
}

int Communicator::size() const {
#ifndef MPI_EXPERIMENTAL
  return 1;
#else
  if (m_backend)
    return m_backend->size();
  return m_communicator.size();
#endif
}

} // namespace Parallel
} // namespace Mantid
