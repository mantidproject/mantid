#ifndef MANTID_INDEXING_INDEXTYPE_H_
#define MANTID_INDEXING_INDEXTYPE_H_

#include <type_traits>

namespace Mantid {
namespace Indexing {
namespace detail {

/** A base class for strongly-typed integers, without implicit conversion.

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
template <class Derived, class Int,
          class = typename std::enable_if<std::is_integral<Int>::value>::type>
class IndexType {
public:
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
