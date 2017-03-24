#include "MantidMPI/CommunicatorBackend.h"

namespace Mantid {
namespace MPI {

CommunicatorBackend::CommunicatorBackend(const int size) : m_size(size) {}

int CommunicatorBackend::size() const { return m_size; }

} // namespace MPI
} // namespace Mantid
