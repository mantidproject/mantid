#ifndef MANTID_INDEXING_SCATTER_H_
#define MANTID_INDEXING_SCATTER_H_

#include "MantidIndexing/DllConfig.h"

namespace Mantid {
namespace Indexing {
class IndexInfo;

/** Scattering for IndexInfo, in particular changing its storage mode to
  Parallel::StorageMode::Distributed.

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
MANTID_INDEXING_DLL IndexInfo scatter(const IndexInfo &indexInfo);

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_SCATTER_H_ */
