#include "MantidParallel/ThreadingBackend.h"

namespace Mantid {
namespace Parallel {
namespace detail {

ThreadingBackend::ThreadingBackend(const int size) : m_size(size) {}

int ThreadingBackend::size() const { return m_size; }

} // namespace detail
} // namespace Parallel
} // namespace Mantid
