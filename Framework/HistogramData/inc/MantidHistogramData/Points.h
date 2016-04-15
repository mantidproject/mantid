#ifndef MANTID_HISTOGRAM_POINTS_H_
#define MANTID_HISTOGRAM_POINTS_H_

#include "MantidHistogram/DllConfig.h"
#include "MantidHistogram/VectorOf.h"
#include "MantidHistogram/ConstIterable.h"

namespace Mantid {
namespace Histogram {

class BinEdges;

/** Points : TODO: DESCRIPTION

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
class MANTID_HISTOGRAM_DLL Points : public VectorOf<Points>,
                                    public ConstIterable<Points> {
public:
  using VectorOf<Points>::VectorOf;
  using VectorOf<Points>::operator=;
  Points() = default;
  Points(const BinEdges &edges);
};

} // namespace Histogram
} // namespace Mantid

#endif /* MANTID_HISTOGRAM_POINTS_H_ */
