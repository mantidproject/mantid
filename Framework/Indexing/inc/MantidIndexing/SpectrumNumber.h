#ifndef MANTID_INDEXING_SPECTRUMNUMBER_H_
#define MANTID_INDEXING_SPECTRUMNUMBER_H_

#include "MantidIndexing/DllConfig.h"

namespace Mantid {
namespace Indexing {

/** SpectrumNumber : TODO: DESCRIPTION

  @author Simon Heybrock
  @date 2016

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
class MANTID_INDEXING_DLL SpectrumNumber {
public:
  explicit SpectrumNumber(int64_t number) noexcept : m_number(number) {}
  template <class T> bool operator==(const T &other) const noexcept {
    return m_number == SpectrumNumber(other).m_number;
  }
  template <class T> bool operator!=(const T &other) const noexcept {
    return m_number != SpectrumNumber(other).m_number;
  }
  // This is explicit, it should help to avoid things like specNum2 = 42 +
  // specNum1?
  explicit operator int64_t() const noexcept { return m_number; }

private:
  int64_t m_number;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_SPECTRUMNUMBER_H_ */
