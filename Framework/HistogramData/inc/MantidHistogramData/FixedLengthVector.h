#ifndef MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_
#define MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace HistogramData {

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
  explicit FixedLengthVector(const Kernel::cow_ptr<std::vector<double>> &data)
      : m_data(data) {
    if (!m_data)
      throw std::logic_error("FixedLengthVector: Cannot init with null data");
  }

  explicit operator bool() const { return m_data.operator bool(); }

  size_t size() const { return m_data->size(); }

  const double &operator[](size_t pos) const { return (*m_data)[pos]; }
  double &operator[](size_t pos) { return m_data.access()[pos]; }

protected:
  // This is used as base class only, cannot delete polymorphically, so
  // destructor is protected.
  ~FixedLengthVector() = default;

  Kernel::cow_ptr<std::vector<double>> m_data;
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTOR_H_ */
