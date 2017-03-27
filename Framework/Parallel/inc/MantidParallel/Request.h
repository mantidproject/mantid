#ifndef MANTID_PARALLEL_REQUEST_H_
#define MANTID_PARALLEL_REQUEST_H_

#include "MantidParallel/DllConfig.h"

#include <boost/mpi/request.hpp>
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
class MANTID_PARALLEL_DLL Request {
public:
  Request() = default;
  Request(const boost::mpi::request &request);

  void wait();

private:
  template <class Function> explicit Request(Function &&f);
  boost::mpi::request m_request;
  std::thread m_thread;
  const bool m_threadingBackend{false};
  // For accessing constructor based on callable.
  friend class detail::ThreadingBackend;
};

template <class Function>
Request::Request(Function &&f)
    : m_thread(std::forward<Function>(f)), m_threadingBackend{true} {}

} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_REQUEST_H_ */
