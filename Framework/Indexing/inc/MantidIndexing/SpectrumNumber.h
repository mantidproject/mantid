#ifndef MANTID_INDEXING_SPECTRUMNUMBER_H_
#define MANTID_INDEXING_SPECTRUMNUMBER_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/IndexType.h"

namespace Mantid {
namespace Indexing {

/** A unique identifier for a spectrum in a workspace.

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
class SpectrumNumber : public detail::IndexType<SpectrumNumber, int32_t> {
public:
  using detail::IndexType<SpectrumNumber, int32_t>::IndexType;
  using detail::IndexType<SpectrumNumber, int32_t>::operator=;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_SPECTRUMNUMBER_H_ */
