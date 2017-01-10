#ifndef MANTID_CURVEFITTING_CRYSTALFIELDPEAKSBASE_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDPEAKSBASE_H_

#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/FortranDefs.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  CrystalFieldPeaks is a function that calculates crystal field peak
  positions and intensities.

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
class MANTID_CURVEFITTING_DLL CrystalFieldPeaksBase
    : public API::ParamFunction {
public:
  CrystalFieldPeaksBase();
  void setAttribute(const std::string &name, const Attribute &) override;

  /// Calculate the crystal field eigensystem
  void calculateEigenSystem(DoubleFortranVector &en, ComplexFortranMatrix &wf,
                            ComplexFortranMatrix &ham, ComplexFortranMatrix &hz,
                            int &nre) const;
  inline void calculateEigenSystem(DoubleFortranVector &en,
                                   ComplexFortranMatrix &wf, int &nre) const {
    ComplexFortranMatrix ham, hz;
    calculateEigenSystem(en, wf, ham, hz, nre);
  }

protected:
  /// Store the default domain size after first
  /// function evaluation
  mutable size_t m_defaultDomainSize;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CRYSTALFIELDPEAKSBASE_H_ */
