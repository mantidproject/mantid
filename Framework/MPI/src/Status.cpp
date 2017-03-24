#include "MantidMPI/Status.h"

namespace Mantid {
namespace MPI {

#ifdef MPI_EXPERIMENTAL
Status::Status(const boost::mpi::status &status) : m_status(status) {}
#else
Status::Status(int source, int tag, int error)
    : m_source(source), m_tag(tag), m_error(error) {}
#endif

int Status::source() const {
#ifdef MPI_EXPERIMENTAL
  return m_status.source();
#else
  return m_source;
#endif
}

int Status::tag() const {
#ifdef MPI_EXPERIMENTAL
  return m_status.tag();
#else
  return m_tag;
#endif
}

int Status::error() const {
#ifdef MPI_EXPERIMENTAL
  return m_status.error();
#else
  return m_error;
#endif
}

} // namespace MPI
} // namespace Mantid
