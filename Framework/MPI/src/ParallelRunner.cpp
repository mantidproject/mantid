#include "MantidMPI/ParallelRunner.h"
#include "MantidMPI/ThreadingBackend.h"

#include <algorithm>

namespace Mantid {
namespace MPI {

ParallelRunner::ParallelRunner() {
#ifndef MPI_EXPERIMENTAL
  // 3 is an arbitrary choice. We need more than 1 since that would be a trivial
  // case, 2 seems like a special case that might make some bugs invisible.
  int threads =
      std::max(3, static_cast<int>(std::thread::hardware_concurrency()));
  m_backend = boost::make_shared<detail::ThreadingBackend>(threads);
#endif
}

ParallelRunner::ParallelRunner(const int threads) {
#ifdef MPI_EXPERIMENTAL
  Communicator comm;
  if (comm.size() != threads)
    throw("ParallelRunner: number of requested threads does not match number "
          "of MPI ranks");
#else
  m_backend = boost::make_shared<detail::ThreadingBackend>(threads);
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
