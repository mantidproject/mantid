#ifndef MANTID_HISTOGRAMDATA_VECTOROF_H_
#define MANTID_HISTOGRAMDATA_VECTOROF_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/make_cow.h"

namespace Mantid {
namespace HistogramData {
namespace detail {

/** VectorOf : TODO: DESCRIPTION

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
template <class T, class CowType> class VectorOf {
public:
  VectorOf() = default;
  VectorOf(size_t count, const double &value) {
    m_data = Kernel::make_cow<CowType>(count, value);
  }
  explicit VectorOf(size_t count) { m_data = Kernel::make_cow<CowType>(count); }
  VectorOf(std::initializer_list<double> init) {
    m_data = Kernel::make_cow<CowType>(init);
  }
  VectorOf(const VectorOf &) = default;
  VectorOf(VectorOf &&) = default;
  // Note the lvalue reference qualifier for all assignment operators. This
  // prevent mistakes in client code, assigning to an rvalue, such as
  // histogram.getBinEdges() = { 0.1, 0.2 };
  VectorOf &operator=(const VectorOf &other) & = default;
  VectorOf &operator=(VectorOf &&other) & = default;

  VectorOf &operator=(std::initializer_list<double> ilist) & {
    m_data = Kernel::make_cow<CowType>(ilist);
    return *this;
  }
  template <class InputIt>
  VectorOf(InputIt first, InputIt last)
      : m_data(Kernel::make_cow<CowType>(first, last)) {}

  // TODO figure out if we want all these overloads.
  explicit VectorOf(const Kernel::cow_ptr<CowType> &other) : m_data(other) {}
  explicit VectorOf(const boost::shared_ptr<CowType> &other) : m_data(other) {}
  // TODO cow_ptr is not movable, can we implement move?
  explicit VectorOf(const CowType &data)
      : m_data(Kernel::make_cow<CowType>(data)) {}
  // VectorOf(std::vector<double> &&data) { m_data =
  // Kernel::make_cow<std::vector<double>>(std::move(data)); }

  VectorOf &operator=(const Kernel::cow_ptr<CowType> &other) & {
    m_data = other;
    return *this;
  }
  VectorOf &operator=(const boost::shared_ptr<CowType> &other) & {
    m_data = other;
    return *this;
  }
  VectorOf &operator=(const CowType &data) & {
    if (!m_data || (&(*m_data) != &data))
      m_data = Kernel::make_cow<CowType>(data);
    return *this;
  }

  explicit operator bool() const { return m_data.operator bool(); }

  size_t size() const { return m_data->size(); }

  // Note that this function returns the internal data of VectorOf, i.e., does
  // not forward to std::vector::data().
  const CowType &data() const { return *m_data; }
  const CowType &constData() const { return *m_data; }
  CowType &data() { return m_data.access(); }
  Kernel::cow_ptr<CowType> cowData() const { return m_data; }
  const std::vector<double> &rawData() const { return m_data->rawData(); }
  const std::vector<double> &constRawData() const { return m_data->rawData(); }
  std::vector<double> &rawData() { return m_data.access().rawData(); }

protected:
  // This is used as base class only, cannot delete polymorphically, so
  // destructor is protected.
  ~VectorOf() = default;

  Kernel::cow_ptr<CowType> m_data{nullptr};
};

} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_VECTOROF_H_ */
