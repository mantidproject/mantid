#ifndef MANTID_PARALLEL_STATUS_H_
#define MANTID_PARALLEL_STATUS_H_

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
class MANTID_PARALLEL_DLL Status {
public:
#ifdef MPI_EXPERIMENTAL
  Status(const boost::mpi::status &status)
      : m_status(status), m_threadingBackend{false} {}
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

#endif /* MANTID_PARALLEL_STATUS_H_ */
