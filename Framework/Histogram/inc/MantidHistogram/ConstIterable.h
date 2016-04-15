#ifndef MANTID_HISTOGRAM_CONSTITERABLE_H_
#define MANTID_HISTOGRAM_CONSTITERABLE_H_

#include "MantidHistogram/DllConfig.h"

namespace Mantid {
namespace Histogram {

/** ConstIterable : TODO: DESCRIPTION

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
template <class T> class ConstIterable {
public:
  const double &operator[](size_t pos) const {
    return static_cast<const T *>(this)->constData()[pos];
  }

protected:
  ~ConstIterable() = default;
};

template <class T>
auto cbegin(const ConstIterable<T> &container)
    -> decltype(static_cast<const T *>(&container)->data().cbegin()) {
  return static_cast<const T *>(&container)->data().cbegin();
}

template <class T>
auto cend(const ConstIterable<T> &container)
    -> decltype(static_cast<const T *>(&container)->data().cend()) {
  return static_cast<const T *>(&container)->data().cend();
}

} // namespace Histogram
} // namespace Mantid

#endif /* MANTID_HISTOGRAM_CONSTITERABLE_H_ */
