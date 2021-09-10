// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidParallel/DllConfig.h"

#include <boost/optional/optional.hpp>
#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/status.hpp>
#endif

namespace Mantid {
namespace Parallel {
namespace detail {
class ThreadingBackend;
}

/** Wrapper for boost::mpi::status. For non-MPI builds an equivalent
  implementation is provided.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_PARALLEL_DLL Status {
public:
#ifdef MPI_EXPERIMENTAL
  Status(const boost::mpi::status &status) : m_status(status), m_threadingBackend{false} {}
#endif

  template <typename T> boost::optional<int> count() const {
#ifdef MPI_EXPERIMENTAL
    if (!m_threadingBackend)
      return m_status.count<T>();
#endif
    return static_cast<int>(m_size / sizeof(T));
  }

private:
  Status(const size_t size) : m_size(size) {}
#ifdef MPI_EXPERIMENTAL
  boost::mpi::status m_status;
#endif
  const size_t m_size{0};
#ifdef MPI_EXPERIMENTAL
  bool m_threadingBackend{true};
#endif
  // For accessing constructor based on size.
  friend class detail::ThreadingBackend;
};

} // namespace Parallel
} // namespace Mantid
