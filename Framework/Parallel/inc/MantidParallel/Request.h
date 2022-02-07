// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidParallel/DllConfig.h"

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/request.hpp>
#endif
#include <thread>

namespace Mantid {
namespace Parallel {
namespace detail {
class ThreadingBackend;
}

/** Wrapper for boost::mpi::request. For non-MPI builds an equivalent
  implementation is provided.

  @author Simon Heybrock
  @date 2017
*/
class MANTID_PARALLEL_DLL Request {
public:
  Request() = default;
#ifdef MPI_EXPERIMENTAL
  Request(const boost::mpi::request &request);
#endif

  void wait();

  bool hasBackend() const { return m_threadingBackend; }

#ifdef MPI_EXPERIMENTAL
  operator boost::mpi::request &() { return m_request; }
#endif

private:
  template <class Function> explicit Request(Function &&f);
#ifdef MPI_EXPERIMENTAL
  boost::mpi::request m_request;
#endif
  std::thread m_thread;
  const bool m_threadingBackend{false};
  // For accessing constructor based on callable.
  friend class detail::ThreadingBackend;
};

template <class Function>
Request::Request(Function &&f) : m_thread(std::forward<Function>(f)), m_threadingBackend{true} {}

} // namespace Parallel
} // namespace Mantid
