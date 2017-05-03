#include "MantidParallel/Request.h"

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/status.hpp>
#endif

namespace Mantid {
namespace Parallel {

#ifdef MPI_EXPERIMENTAL
Request::Request(const boost::mpi::request &request) : m_request(request) {}
#endif

void Request::wait() {
  // Not returning a status since it would usually not get initialized. See
  // http://mpi-forum.org/docs/mpi-1.1/mpi-11-html/node35.html#Node35.
  if (m_threadingBackend)
    if (m_thread.joinable())
      m_thread.join();
#ifdef MPI_EXPERIMENTAL
  static_cast<void>(m_request.wait());
#endif
}

} // namespace Parallel
} // namespace Mantid
