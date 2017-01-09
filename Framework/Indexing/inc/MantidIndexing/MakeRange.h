#ifndef MANTID_INDEXING_MAKERANGE_H_
#define MANTID_INDEXING_MAKERANGE_H_

#include "MantidIndexing/DllConfig.h"

#include <numeric>
#include <type_traits>

namespace Mantid {
namespace Indexing {

/** Helper function for generating a vector with a range of integers, similar to
  Python's range(). Return a vector of integers starting at 'first' and eding
  with 'last' with increments of 1.

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
template <class T1, class T2,
          class = typename std::enable_if<std::is_integral<T1>::value &&
                                          std::is_integral<T2>::value>::type>
std::vector<T2> makeRange(T1 first, T2 last) {
  std::vector<T2> vec(last - static_cast<T2>(first) + 1);
  std::iota(vec.begin(), vec.end(), static_cast<T2>(first));
  return vec;
}

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_MAKERANGE_H_ */
