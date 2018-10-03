// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_INDEXTYPE_H_
#define MANTID_INDEXING_INDEXTYPE_H_

#include <type_traits>

namespace Mantid {
namespace Indexing {
namespace detail {

/** A base class for strongly-typed integers, without implicit conversion.

  @author Simon Heybrock
  @date 2017
*/
template <class Derived, class Int,
          class = typename std::enable_if<std::is_integral<Int>::value>::type>
class IndexType {
public:
  using underlying_type = Int;
  IndexType() noexcept : m_data(0) {}
  IndexType(Int data) noexcept : m_data(data) {}
  explicit operator Int() const noexcept { return m_data; }
  template <class T> IndexType &operator=(const T &other) noexcept {
    m_data = IndexType(other).m_data;
    return *this;
  }
  template <class T> bool operator==(const T &other) const noexcept {
    return m_data == IndexType(other).m_data;
  }
  template <class T> bool operator!=(const T &other) const noexcept {
    return m_data != IndexType(other).m_data;
  }
  template <class T> bool operator>(const T &other) const noexcept {
    return m_data > IndexType(other).m_data;
  }
  template <class T> bool operator>=(const T &other) const noexcept {
    return m_data >= IndexType(other).m_data;
  }
  template <class T> bool operator<(const T &other) const noexcept {
    return m_data < IndexType(other).m_data;
  }
  template <class T> bool operator<=(const T &other) const noexcept {
    return m_data <= IndexType(other).m_data;
  }

private:
  Int m_data;
};

} // namespace detail
} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_INDEXTYPE_H_ */
