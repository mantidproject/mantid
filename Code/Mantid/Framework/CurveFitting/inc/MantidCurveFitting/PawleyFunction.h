#ifndef MANTID_CURVEFITTING_PAWLEYFUNCTION_H_
#define MANTID_CURVEFITTING_PAWLEYFUNCTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid {
namespace CurveFitting {

/** PawleyFunction

  The Pawley approach to obtain lattice parameters from a powder diffractogram
  works by placing peak profiles at d-values (which result from the lattice
  parameters and the Miller indices of each peak) and fitting the total profile
  to the recorded diffractogram.

  Depending on the chosen crystal system, this function exposes the appropriate
  lattice parameters as parameters, as well as profile parameters of the
  individual peak functions, except the peak locations, which are a direct
  result of their HKLs in combination with the unit cell.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 11/03/2015

  Copyright © 2015 PSI-NXMM

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
class DLLExport PawleyParameterFunction : virtual public API::IFunction,
                                          virtual public API::ParamFunction {
public:
  PawleyParameterFunction();
  virtual ~PawleyParameterFunction() {}

  std::string name() const { return "PawleyParameterFunction"; }

  void setAttribute(const std::string &attName, const Attribute &attValue);

  Geometry::PointGroup::CrystalSystem getCrystalSystem() const;
  Geometry::UnitCell getUnitCellFromParameters() const;

  void function(const API::FunctionDomain &domain,
                API::FunctionValues &values) const;
  void functionDeriv(const API::FunctionDomain &domain,
                     API::Jacobian &jacobian);

protected:
  void init();

  void setCrystalSystem(const std::string &crystalSystem);

  void createCrystalSystemParameters(
      Geometry::PointGroup::CrystalSystem crystalSystem);

  Geometry::PointGroup::CrystalSystem m_crystalSystem;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_PAWLEYFUNCTION_H_ */
