#ifndef MANTID_CURVEFITTING_LATTICEFUNCTION_H_
#define MANTID_CURVEFITTING_LATTICEFUNCTION_H_

#include "MantidAPI/ILatticeFunction.h"
#include "MantidKernel/System.h"

#include "MantidCurveFitting/Functions/PawleyFunction.h"

namespace Mantid {
namespace CurveFitting {

/** LatticeFunction

  LatticeFunction implements API::ILatticeFunction. Internally it uses
  a PawleyParameterFunction to expose appropriate lattice parameters for each
  crystal system.

  For each HKL in the supplied LatticeDomain, the function method calculates
  the d-value using the UnitCell that is generated from the function parameters.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/04/2015

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
class DLLExport LatticeFunction : public API::ILatticeFunction {
public:
  LatticeFunction();

  std::string name() const override { return "LatticeFunction"; }

  void functionLattice(const API::LatticeDomain &latticeDomain,
                       API::FunctionValues &values) const override;

  void setLatticeSystem(const std::string &crystalSystem) override;
  void setUnitCell(const std::string &unitCellString) override;
  void setUnitCell(const Geometry::UnitCell &unitCell) override;
  Geometry::UnitCell getUnitCell() const override;

protected:
  void init() override;
  void beforeDecoratedFunctionSet(const API::IFunction_sptr &fn) override;

private:
  Functions::PawleyParameterFunction_sptr m_cellParameters;
};

using LatticeFunction_sptr = boost::shared_ptr<LatticeFunction>;

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_LATTICEFUNCTION_H_ */
