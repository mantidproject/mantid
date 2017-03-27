#include "MantidParallel/Communicator.h"

namespace Mantid {
namespace Parallel {

Communicator::Communicator(const boost::mpi::communicator &comm)
    : m_communicator(comm) {}

Communicator::Communicator(boost::shared_ptr<detail::ThreadingBackend> backend,
                           const int rank)
    : m_backend(backend), m_rank(rank) {}

int Communicator::rank() const {
  if (m_backend)
    return m_rank;
  return m_communicator.rank();
}

int Communicator::size() const {
  if (m_backend)
    return m_backend->size();
  return m_communicator.size();
}

} // namespace Parallel
} // namespace Mantid
