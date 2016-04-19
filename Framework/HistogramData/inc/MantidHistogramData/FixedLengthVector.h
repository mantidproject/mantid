#ifndef MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_
#define MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_

#include "MantidHistogramData/DllConfig.h"

#include <stdexcept>
#include <vector>

namespace Mantid {
namespace HistogramData {

class Histogram;

namespace detail {

/** HistogramData : TODO: DESCRIPTION

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

  FixedLengthVector &operator=(const FixedLengthVector &rhs) {
    if (size() != rhs.size())
      throw std::logic_error("FixedLengthVector::operator=: size mismatch");
    m_data = rhs.m_data;
    return *this;
  }
  FixedLengthVector &operator=(FixedLengthVector &&rhs) {
    if (size() != rhs.size())
      throw std::logic_error("FixedLengthVector::operator=: size mismatch");
    m_data = std::move(rhs.m_data);
    return *this;
  }
  FixedLengthVector &operator=(const std::vector<double> &rhs) {
    if (size() != rhs.size())
      throw std::logic_error("FixedLengthVector::operator=: size mismatch");
    m_data = rhs;
    return *this;
  }
  FixedLengthVector &operator=(std::vector<double> &&rhs) {
    if (size() != rhs.size())
      throw std::logic_error("FixedLengthVector::operator=: size mismatch");
    m_data = std::move(rhs);
    return *this;
  }
  FixedLengthVector &operator=(std::initializer_list<double> ilist) {
    if (size() != ilist.size())
      throw std::logic_error("FixedLengthVector::operator=: size mismatch");
    m_data = ilist;
    return *this;
  }

  size_t size() const { return m_data.size(); }

  const double &operator[](size_t pos) const { return m_data[pos]; }
  double &operator[](size_t pos) { return m_data[pos]; }

protected:
  const std::vector<double> &rawData() const { return m_data; }
  std::vector<double> &rawData() { return m_data; }

  // This is used as base class only, cannot delete polymorphically, so
  // destructor is protected.
  ~FixedLengthVector() = default;

  std::vector<double> m_data;

public:
  auto cbegin() const -> decltype(m_data.cbegin()) {
    return m_data.cbegin();
  }

  auto cend() const -> decltype(m_data.cend()) {
    return m_data.cend();
  }
};

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_ */
