#include "MantidMPI/Request.h"

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/status.hpp>
#endif

namespace Mantid {
namespace MPI {

#ifdef MPI_EXPERIMENTAL
Request::Request(const boost::mpi::request &request) : m_request(request) {}
#endif

void Request::wait() {
// Not returning a status since it would usually not get initialized. See
// http://mpi-forum.org/docs/mpi-1.1/mpi-11-html/node35.html#Node35.
#ifdef MPI_EXPERIMENTAL
  static_cast<void>(m_request.wait());
#else
  if (m_thread.joinable())
    m_thread.join();
#endif
}

} // namespace MPI
} // namespace Mantid
