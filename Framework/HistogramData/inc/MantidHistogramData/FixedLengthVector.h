#ifndef MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_
#define MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/Validation.h"

#include <numeric>
#include <limits>
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
  FixedLengthVector(std::initializer_list<double> init) : m_data(init) {
    Validator<T>::checkValidity(m_data);
  }
  FixedLengthVector(const FixedLengthVector &) = default;
  FixedLengthVector(FixedLengthVector &&) = default;
  FixedLengthVector(const std::vector<double> &other) : m_data(other) {}
  FixedLengthVector(std::vector<double> &&other) : m_data(std::move(other)) {}
  template <class InputIt>
  FixedLengthVector(InputIt first, InputIt last)
      : m_data(first, last) {}
  template <class Generator,
            class = typename std::enable_if<
                !std::is_convertible<Generator, double>::value>::type>
  FixedLengthVector(size_t count, const Generator &g)
      : m_data(count) {
    std::generate(m_data.begin(), m_data.end(), g);
  }

  template <class InputIt> void assign(InputIt first, InputIt last) & {
    checkAssignmentSize(static_cast<size_t>(std::distance(first, last)));
    m_data.assign(first, last);
  }
  void assign(size_t count, const double &value) & {
    checkAssignmentSize(count);
    m_data.assign(count, value);
  }

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
    Validator<T>::checkValidity(ilist);
    m_data = ilist;
    return *this;
  }
  FixedLengthVector &operator=(const double value) {
    m_data.assign(m_data.size(), value);
    return *this;
  }

  bool empty() const { return m_data.empty(); }
  size_t size() const { return m_data.size(); }

  const double &operator[](size_t pos) const { return m_data[pos]; }
  double &operator[](size_t pos) { return m_data[pos]; }

  /// Returns a const reference to the underlying vector.
  const std::vector<double> &rawData() const { return m_data; }

  /// Returns the sum over a range of values from min (inclusive) to max
  /// (exclusive)
  double sum(size_t min = 0, size_t max = std::numeric_limits<size_t>::max(),
             double initialValue = 0.0) const {
    max = std::min(max, size());
    return std::accumulate(begin() + min, begin() + max, initialValue);
  }

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
  template <class Other> void checkAssignmentSize(const Other &other) const {
    if (size() != other.size())
      throw std::logic_error("FixedLengthVector::operator=: size mismatch");
  }

  void checkAssignmentSize(const size_t &size) const {
    if (this->size() != size)
      throw std::logic_error("FixedLengthVector::assign: size mismatch");
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
  auto crbegin() const -> decltype(m_data.crbegin()) {
    return m_data.crbegin();
  }
  auto crend() const -> decltype(m_data.crend()) { return m_data.crend(); }
  double &front() { return m_data.front(); }
  double &back() { return m_data.back(); }
  const double &front() const { return m_data.front(); }
  const double &back() const { return m_data.back(); }

  // expose typedefs for the iterator types in the underlying container
  typedef std::vector<double>::iterator iterator;
  typedef std::vector<double>::const_iterator const_iterator;
};

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_ */
