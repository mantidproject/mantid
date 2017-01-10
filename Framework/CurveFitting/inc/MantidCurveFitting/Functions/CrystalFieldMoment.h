#ifndef MANTID_CURVEFITTING_CRYSTALFIELDMOMENT_H
#define MANTID_CURVEFITTING_CRYSTALFIELDMOMENT_H

#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"
#include "MantidCurveFitting/FortranDefs.h"
#include "MantidAPI/IFunction1D.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  CrystalFieldMoment is a function that calculates the induced
  magnetic moment (in bohr magnetons per ion) at some applied external
  magnetic field (in Tesla) as a function of temperature (in Kelvin).
  for a particular crystal field splitting.

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

class MANTID_CURVEFITTING_DLL CrystalFieldMoment : public CrystalFieldPeaksBase,
                                                   public API::IFunction1D {
public:
  CrystalFieldMoment();
  std::string name() const override { return "CrystalFieldMoment"; }
  const std::string category() const override { return "General"; }
  void set_hamiltonian(const ComplexFortranMatrix &wf_in, const int nre_in);
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

private:
  ComplexFortranMatrix ham;
  int nre;
  bool setDirect;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CRYSTALFIELDMOMENT_H */
