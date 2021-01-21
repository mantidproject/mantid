// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/Request.h"

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/nonblocking.hpp>
#endif

namespace Mantid {
namespace Parallel {

/** Wrapper for boost::mpi::nonblocking. For non-MPI builds an equivalent
  implementation with reduced functionality is provided.

  @author Simon Heybrock
  @date 2017
*/
template <typename ForwardIterator> void wait_all(ForwardIterator begin, ForwardIterator end) {
#ifdef MPI_EXPERIMENTAL
  class RequestIteratorWrapper : public ForwardIterator {
  public:
    RequestIteratorWrapper(const ForwardIterator &it) : ForwardIterator(it) {}
    boost::mpi::request &operator*() { return ForwardIterator::operator*(); }
    boost::mpi::request *operator->() { return &operator*(); }
  };
  if (begin == end || !begin->hasBackend())
    return boost::mpi::wait_all(RequestIteratorWrapper(begin), RequestIteratorWrapper(end));
#endif
  while (begin != end) {
    (*begin).wait();
    ++begin;
  }
}

} // namespace Parallel
} // namespace Mantid
