#include "MantidMPI/ThreadingBackend.h"

namespace Mantid {
namespace MPI {

ThreadingBackend::ThreadingBackend(const int size) : m_size(size) {}

int ThreadingBackend::size() const { return m_size; }

} // namespace MPI
} // namespace Mantid
