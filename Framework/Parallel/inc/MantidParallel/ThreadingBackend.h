// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARALLEL_THREADINGBACKEND_H_
#define MANTID_PARALLEL_THREADINGBACKEND_H_

#include "MantidParallel/DllConfig.h"
#include "MantidParallel/Request.h"
#include "MantidParallel/Status.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <chrono>
#include <functional>
#include <istream>
#include <map>
#include <mutex>
#include <ostream>
#include <sstream>
#include <tuple>
#include <vector>

namespace Mantid {
namespace Parallel {
namespace detail {

/** ThreadingBackend provides a backend for data exchange between Communicators
  in the case of non-MPI builds when communication between threads is used to
  mimic MPI calls. This is FOR UNIT TESTING ONLY and is NOT FOR PRODUCTION CODE.

  @author Simon Heybrock
  @date 2017
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
  Status recv(int dest, int source, int tag, T &&... args);

  template <typename... T>
  Request isend(int source, int dest, int tag, T &&... args);

  template <typename T> Request irecv(int dest, int source, int tag, T &&data);
  template <typename T>
  Request irecv(int dest, int source, int tag, T *data, const size_t count);

private:
  int m_size{1};
  std::map<std::tuple<int, int, int>,
           std::vector<std::unique_ptr<std::stringbuf>>>
      m_buffer;
  std::mutex m_mutex;
};

namespace detail {
template <class T>
void saveToStream(boost::archive::binary_oarchive &oa, const T &data) {
  oa.operator<<(data);
}
template <class T>
void saveToStream(boost::archive::binary_oarchive &oa,
                  const std::vector<T> &data) {
  oa.operator<<(data);
}
template <class T>
void saveToStream(boost::archive::binary_oarchive &oa, const T *data,
                  const size_t count) {
  oa.operator<<(count);
  for (size_t i = 0; i < count; ++i)
    oa.operator<<(data[i]);
}
template <class T>
size_t loadFromStream(boost::archive::binary_iarchive &ia, T &data) {
  ia.operator>>(data);
  return sizeof(T);
}
template <class T>
size_t loadFromStream(boost::archive::binary_iarchive &ia,
                      std::vector<T> &data) {
  ia.operator>>(data);
  return data.size() * sizeof(T);
}
template <class T>
size_t loadFromStream(boost::archive::binary_iarchive &ia, T *data,
                      const size_t count) {
  size_t received;
  ia.operator>>(received);
  for (size_t i = 0; i < count; ++i) {
    if (i >= received)
      return i * sizeof(T);
    ia.operator>>(data[i]);
  }
  return count * sizeof(T);
}
} // namespace detail

template <typename... T>
void ThreadingBackend::send(int source, int dest, int tag, T &&... args) {
  // Must wrap std::stringbuf in a unique_ptr since gcc on RHEL7 does not
  // support moving a stringbuf (incomplete C++11 support?).
  auto buf = std::make_unique<std::stringbuf>();
  std::ostream os(buf.get());
  {
    // The binary_oarchive must be scoped to prevent a segmentation fault. I
    // believe the reason is that otherwise recv() may end up reading from the
    // buffer while the oarchive is still alive. I do not really understand this
    // though, since it is *not* writing to the buffer, somehow the oarchive
    // destructor must be doing something that requires the buffer.
    boost::archive::binary_oarchive oa(os);
    detail::saveToStream(oa, std::forward<T>(args)...);
  }
  std::lock_guard<std::mutex> lock(m_mutex);
  m_buffer[std::make_tuple(source, dest, tag)].push_back(std::move(buf));
}

template <typename... T>
Status ThreadingBackend::recv(int dest, int source, int tag, T &&... args) {
  const auto key = std::make_tuple(source, dest, tag);
  std::unique_ptr<std::stringbuf> buf;
  while (true) {
    // Sleep to reduce lock contention. Without this execution times can grow
    // enormously on Windows.
    std::this_thread::sleep_for(std::chrono::microseconds(10));
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
  return Status(detail::loadFromStream(ia, std::forward<T>(args)...));
}

template <typename... T>
Request ThreadingBackend::isend(int source, int dest, int tag, T &&... args) {
  send(source, dest, tag, std::forward<T>(args)...);
  return Request{};
}

template <typename T>
Request ThreadingBackend::irecv(int dest, int source, int tag, T &&data) {
  return Request(std::bind(&ThreadingBackend::recv<T>, this, dest, source, tag,
                           std::ref(std::forward<T>(data))));
}
template <typename T>
Request ThreadingBackend::irecv(int dest, int source, int tag, T *data,
                                const size_t count) {
  // Pass (pointer) by value since reference to it may go out of scope.
  return Request([this, dest, source, tag, data, count]() mutable {
    recv(dest, source, tag, data, count);
  });
}

} // namespace detail
} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_THREADINGBACKEND_H_ */
