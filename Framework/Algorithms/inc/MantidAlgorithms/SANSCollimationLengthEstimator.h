#ifndef MANTID_ALGORITHMS_SANSCOLLISIONLENGTHESTIMATOR_H
#define MANTID_ALGORITHMS_SANSCOLLISIONLENGTHESTIMATOR_H

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/System.h"
/**Helper class which provides the Collimation Length for SANS instruments


Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge
National Laboratory

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
namespace Mantid {
namespace Algorithms {
class DLLExport SANSCollimationLengthEstimator {
public:
  double provideCollimationLength(Mantid::API::MatrixWorkspace_sptr workspace);

private:
  double getCollimationLengthWithGuides(
      Mantid::API::MatrixWorkspace_sptr inOutWS, const double L1,
      const double collimationLengthCorrection) const;
  double getGuideValue(Mantid::Kernel::Property *prop) const;
};
} // namespace Algorithms
} // namespace Mantid
#endif
