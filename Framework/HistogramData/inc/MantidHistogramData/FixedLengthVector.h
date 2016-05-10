#ifndef MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_
#define MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_

#include "MantidHistogramData/DllConfig.h"

#include <stdexcept>
#include <vector>

namespace Mantid {
namespace HistogramData {

class Histogram;

namespace detail {

/** FixedLengthVector

  Base class providing an object similar to std::vector, with the key difference
  that the length cannot be changed after creation. This is an implementation
  detail of HistogramData::Histogram, HistogramData::BinEdges, and
  HistogramData::Points and is not intended for direct use in client code.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
template <class T> class FixedLengthVector {
public:
  FixedLengthVector() = default;
  FixedLengthVector(size_t count, const double &value) : m_data(count, value) {}
  explicit FixedLengthVector(size_t count) : m_data(count) {}
  FixedLengthVector(std::initializer_list<double> init) : m_data(init) {}
  FixedLengthVector(const FixedLengthVector &) = default;
  FixedLengthVector(FixedLengthVector &&) = default;
  FixedLengthVector(const std::vector<double> &other) : m_data(other) {}
  FixedLengthVector(std::vector<double> &&other) : m_data(std::move(other)) {}
  template <class InputIt>
  FixedLengthVector(InputIt first, InputIt last)
      : m_data(first, last) {}

  FixedLengthVector &operator=(const FixedLengthVector &rhs) {
    checkAssignmentSize(rhs);
    m_data = rhs.m_data;
    return *this;
  }
  FixedLengthVector &operator=(FixedLengthVector &&rhs) {
    checkAssignmentSize(rhs);
    m_data = std::move(rhs.m_data);
    return *this;
  }
  FixedLengthVector &operator=(const std::vector<double> &rhs) {
    checkAssignmentSize(rhs);
    m_data = rhs;
    return *this;
  }
  FixedLengthVector &operator=(std::vector<double> &&rhs) {
    checkAssignmentSize(rhs);
    m_data = std::move(rhs);
    return *this;
  }
  FixedLengthVector &operator=(std::initializer_list<double> ilist) {
    checkAssignmentSize(ilist);
    m_data = ilist;
    return *this;
  }

  size_t size() const { return m_data.size(); }

  const double &operator[](size_t pos) const { return m_data[pos]; }
  double &operator[](size_t pos) { return m_data[pos]; }

  /// Returns a const reference to the underlying vector.
  const std::vector<double> &rawData() const { return m_data; }

protected:
  /** Returns a reference to the underlying vector.
   *
   * Note that this is not available in the public interface, since that would
   * allow for length modifications, which we need to prevent. */
  std::vector<double> &mutableRawData() { return m_data; }

  // This is used as base class only, cannot delete polymorphically, so
  // destructor is protected.
  ~FixedLengthVector() = default;

private:
  template <class Other> void checkAssignmentSize(const Other &other) {
    if (size() != other.size())
      throw std::logic_error("FixedLengthVector::operator=: size mismatch");
  }

  std::vector<double> m_data;

public:
  auto begin() -> decltype(m_data.begin()) { return m_data.begin(); }
  auto end() -> decltype(m_data.end()) { return m_data.end(); }
  auto begin() const -> decltype(m_data.begin()) { return m_data.begin(); }
  auto end() const -> decltype(m_data.end()) { return m_data.end(); }
  auto cbegin() const -> decltype(m_data.cbegin()) { return m_data.cbegin(); }
  auto cend() const -> decltype(m_data.cend()) { return m_data.cend(); }
  auto rbegin() -> decltype(m_data.rbegin()) { return m_data.rbegin(); }
  auto rend() -> decltype(m_data.rend()) { return m_data.rend(); }
  auto rbegin() const -> decltype(m_data.rbegin()) { return m_data.rbegin(); }
  auto rend() const -> decltype(m_data.rend()) { return m_data.rend(); }
  auto crbegin() const -> decltype(m_data.crbegin()) { return m_data.crbegin(); }
  auto crend() const -> decltype(m_data.crend()) { return m_data.crend(); }
};

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_ */
