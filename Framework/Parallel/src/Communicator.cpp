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

} // namespace Parallel
} // namespace Mantid
