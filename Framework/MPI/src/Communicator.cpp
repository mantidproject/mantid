#include "MantidMPI/Communicator.h"

namespace Mantid {
namespace MPI {

#ifdef MPI_EXPERIMENTAL
Communicator::Communicator(const boost::mpi::communicator &comm)
    : m_communicator(comm) {}
#else
Communicator::Communicator(boost::shared_ptr<ThreadingBackend> backend,
                           const int rank)
    : m_backend(backend), m_rank(rank) {}
#endif

int Communicator::rank() const {
#ifdef MPI_EXPERIMENTAL
  return m_communicator.rank();
#else
  return m_rank;
#endif
}

int Communicator::size() const {
#ifdef MPI_EXPERIMENTAL
  return m_communicator.size();
#else
  return m_backend->size();
#endif
}

} // namespace MPI
} // namespace Mantid
