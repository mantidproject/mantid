#include "MantidTestHelpers/ParallelRunner.h"
#include "MantidParallel/ThreadingBackend.h"

#include <algorithm>

using namespace Mantid::Parallel;

namespace ParallelTestHelpers {

ParallelRunner::ParallelRunner() {
  Communicator comm;
  // Fake parallelism via threads if there is only 1 MPI rank
  if (comm.size() == 1) {
    // 3 is an arbitrary choice. We need more than 1 since that would be a
    // trivial case, 2 seems like a special case that might make some bugs
    // invisible.
    int threads =
        std::max(3, static_cast<int>(std::thread::hardware_concurrency()));
    m_backend = boost::make_shared<detail::ThreadingBackend>(threads);
  }
}

ParallelRunner::ParallelRunner(const int threads) {
  Communicator comm;
  // Fake parallelism via threads if there is only 1 MPI rank, but fail if there
  // are MPI ranks that do not match the number of requested threads.
  if (comm.size() != 1 && comm.size() != threads)
    throw("ParallelRunner: number of requested threads does not match number "
          "of MPI ranks");
  m_backend = boost::make_shared<detail::ThreadingBackend>(threads);
}

int ParallelRunner::size() const {
  if (m_backend)
    return m_backend->size();
  Communicator comm;
  return comm.size();
}

} // namespace ParallelTestHelpers
