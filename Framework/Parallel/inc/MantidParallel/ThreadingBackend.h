#ifndef MANTID_PARALLEL_THREADINGBACKEND_H_
#define MANTID_PARALLEL_THREADINGBACKEND_H_

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/Request.h"
#include "MantidKernel/make_unique.h"

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <functional>
#include <istream>
#include <map>
#include <mutex>
#include <ostream>
#include <sstream>
#include <tuple>

namespace Mantid {
namespace Parallel {
namespace detail {

/** ThreadingBackend provides a backend for data exchange between Communicators
  in the case of non-MPI builds when communication between threads is used to
  mimic MPI calls. This is FOR UNIT TESTING ONLY and is NOT FOR PRODUCTION CODE.

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
class MANTID_PARALLEL_DLL ThreadingBackend {
public:
  ThreadingBackend() = default;
  explicit ThreadingBackend(const int size);

  ThreadingBackend(const ThreadingBackend &) = delete;
  const ThreadingBackend &operator=(const ThreadingBackend &) = delete;

  int size() const;

  template <typename... T>
  void send(int source, int dest, int tag, T &&... args);

  template <typename... T>
  void recv(int dest, int source, int tag, T &&... args);

  template <typename... T>
  Request isend(int source, int dest, int tag, T &&... args);

  template <typename... T>
  Request irecv(int dest, int source, int tag, T &&... args);

private:
  int m_size{1};
  std::map<std::tuple<int, int, int>,
           std::vector<std::unique_ptr<std::stringbuf>>> m_buffer;
  std::mutex m_mutex;
};

template <typename... T>
void ThreadingBackend::send(int source, int dest, int tag, T &&... args) {
  // Must wrap std::stringbuf in a unique_ptr since gcc on RHEL7 does not
  // support moving a stringbuf (incomplete C++11 support?).
  auto buf = Kernel::make_unique<std::stringbuf>();
  std::ostream os(buf.get());
  {
    // The binary_oarchive must be scoped to prevent a segmentation fault. I
    // believe the reason is that otherwise recv() may end up reading from the
    // buffer while the oarchive is still alive. I do not really understand this
    // though, since it is *not* writing to the buffer, somehow the oarchive
    // destructor must be doing something that requires the buffer.
    boost::archive::binary_oarchive oa(os);
    oa.operator<<(std::forward<T>(args)...);
  }
  std::lock_guard<std::mutex> lock(m_mutex);
  m_buffer[std::make_tuple(source, dest, tag)].push_back(std::move(buf));
}

template <typename... T>
void ThreadingBackend::recv(int dest, int source, int tag, T &&... args) {
  const auto key = std::make_tuple(source, dest, tag);
  std::unique_ptr<std::stringbuf> buf;
  while (true) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_buffer.find(key);
    if (it == m_buffer.end())
      continue;
    auto &queue = it->second;
    if (queue.empty())
      continue;
    buf = std::move(queue.front());
    queue.erase(queue.begin());
    break;
  }
  std::istream is(buf.get());
  boost::archive::binary_iarchive ia(is);
  ia.operator>>(std::forward<T>(args)...);
}

template <typename... T>
Request ThreadingBackend::isend(int source, int dest, int tag, T &&... args) {
  send(source, dest, tag, std::forward<T>(args)...);
  return Request{};
}

template <typename... T>
Request ThreadingBackend::irecv(int dest, int source, int tag, T &&... args) {
  return Request(std::bind(&ThreadingBackend::recv<T...>, this, dest, source,
                           tag, std::ref(std::forward<T>(args)...)));
}

} // namespace detail
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_THREADINGBACKEND_H_ */
