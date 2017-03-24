#include "MantidMPI/ThreadingBackend.h"

namespace Mantid {
namespace MPI {
namespace detail {

ThreadingBackend::ThreadingBackend(const int size) : m_size(size) {}

int ThreadingBackend::size() const { return m_size; }

} // namespace detail
} // namespace MPI
} // namespace Mantid
