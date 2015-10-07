#ifndef MANTID_ALGORITHMS_SANSCOLLISIONLENGTHESTIMATOR_H
#define MANTID_ALGORITHMS_SANSCOLLISIONLENGTHESTIMATOR_H

#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
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
  SANSCollimationLengthEstimator();
  ~SANSCollimationLengthEstimator();
  double provideCollimationLength(Mantid::API::MatrixWorkspace_sptr workspace);

private:
  double getCollimationLengthWithGuides(
      Mantid::API::MatrixWorkspace_sptr inOutWS, const double L1,
      const double collimationLengthCorrection) const;
};
}
}
#endif