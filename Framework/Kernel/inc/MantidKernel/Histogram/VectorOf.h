#ifndef MANTID_KERNEL_HISTOGRAM_VECTOROF_H_
#define MANTID_KERNEL_HISTOGRAM_VECTOROF_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/make_cow.h"

namespace Mantid {
namespace Kernel {

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
template <class T> class MANTID_KERNEL_DLL VectorOf {
public:
  VectorOf() = default;
  VectorOf(size_t count, const double &value) {
    m_data = make_cow<std::vector<double>>(count, value);
  }
  explicit VectorOf(size_t count) {
    m_data = make_cow<std::vector<double>>(count);
  }
  VectorOf(std::initializer_list<double> init) {
    m_data = make_cow<std::vector<double>>(init);
  }
  VectorOf(const VectorOf &other) = default;
  VectorOf(VectorOf &&other) = default;
  VectorOf &operator=(const VectorOf &other) = default;
  VectorOf &operator=(VectorOf &&other) = default;

  VectorOf &operator=(std::initializer_list<double> ilist) {
    m_data = make_cow<std::vector<double>>(ilist);
    return *this;
  }

  // TODO figure out if we want all these overloads.
  VectorOf(const cow_ptr<std::vector<double>> &other) { m_data = other; }
  VectorOf(const boost::shared_ptr<std::vector<double>> &other) {
    m_data = other;
  }
  // TODO cow_ptr is not movable, can we implement move?
  VectorOf(const std::vector<double> &data) {
    m_data = make_cow<std::vector<double>>(data);
  }
  // VectorOf(std::vector<double> &&data) { m_data =
  // make_cow<std::vector<double>>(std::move(data)); }

  explicit operator bool() const { return m_data.operator bool(); }

  size_t size() const { return m_data->size(); }
  void resize(size_t size) { m_data.access().resize(size); }

  // Note that this function returns the internal data of VectorOf, i.e., does
  // not forward to std::vector::data().
  const std::vector<double> &data() const { return *m_data; }
  const std::vector<double> &constData() const { return *m_data; }
  std::vector<double> &data() { return m_data.access(); }
  cow_ptr<std::vector<double>> cowData() const { return m_data; }

protected:
  // This is used as base class only, cannot delete polymorphically, so
  // destructor is protected.
  ~VectorOf() = default;

  cow_ptr<std::vector<double>> m_data{nullptr};
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_HISTOGRAM_VECTOROF_H_ */
