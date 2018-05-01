#ifndef MANTID_PARALLEL_NONBLOCKING_H_
#define MANTID_PARALLEL_NONBLOCKING_H_

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

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <typename ForwardIterator>
void wait_all(ForwardIterator begin, ForwardIterator end) {
#ifdef MPI_EXPERIMENTAL
  class RequestIteratorWrapper : public ForwardIterator {
  public:
    RequestIteratorWrapper(const ForwardIterator &it) : ForwardIterator(it) {}
    boost::mpi::request &operator*() { return ForwardIterator::operator*(); }
    boost::mpi::request *operator->() { return &operator*(); }
  };
  if (begin == end || !begin->hasBackend())
    return boost::mpi::wait_all(RequestIteratorWrapper(begin),
                                RequestIteratorWrapper(end));
#endif
  while (begin != end) {
    (*begin).wait();
    ++begin;
  }
}

} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_NONBLOCKING_H_ */
