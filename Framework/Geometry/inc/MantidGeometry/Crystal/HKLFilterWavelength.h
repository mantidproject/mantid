#ifndef MANTID_GEOMETRY_HKLFILTERWAVELENGTH_H_
#define MANTID_GEOMETRY_HKLFILTERWAVELENGTH_H_

#include "MantidGeometry/Crystal/HKLFilter.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Geometry {

/** HKLFilterWavelength

  This implementation of HKLFilter filters reflections by a wavelength-
  range. The wavelength is calculated from the Q-vector, so the filter
  requires an orientation matrix.

      @author Michael Wedel, ESS
      @date 23/10/2015

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_GEOMETRY_DLL HKLFilterWavelength : public HKLFilter {
public:
  HKLFilterWavelength(const Kernel::DblMatrix &ub, double lambdaMin,
                      double lambdaMax);

  std::string getDescription() const override;
  bool isAllowed(const Kernel::V3D &hkl) const override;

protected:
  void checkProperLambdaRangeValues() const;

  Kernel::DblMatrix m_ub;
  double m_lambdaMin;
  double m_lambdaMax;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_HKLFILTERWAVELENGTH_H_ */
