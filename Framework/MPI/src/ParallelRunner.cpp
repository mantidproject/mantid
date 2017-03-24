#include "MantidMPI/ParallelRunner.h"

#include <omp.h>

namespace Mantid {
namespace MPI {

ParallelRunner::ParallelRunner() {
#ifndef MPI_EXPERIMENTAL
  m_backend = boost::make_shared<CommunicatorBackend>(omp_get_max_threads());
#endif
}

ParallelRunner::ParallelRunner(const int threads) {
#ifdef MPI_EXPERIMENTAL
  Communicator comm;
  if (comm.size() != threads)
    throw(
        "ParallelRunner: number of requested threads does not match number of "
        "MPI ranks");
#else
  m_backend = boost::make_shared<CommunicatorBackend>(threads);
#endif
}

int ParallelRunner::size() const {
#ifdef MPI_EXPERIMENTAL
  Communicator comm;
  return comm.size();
#else
  return m_backend->size();
#endif
}

} // namespace MPI
} // namespace Mantid
