#ifndef MANTID_CURVEFITTING_LATTICEFUNCTION_H_
#define MANTID_CURVEFITTING_LATTICEFUNCTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ILatticeFunction.h"

#include "MantidCurveFitting/PawleyFunction.h"

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
  virtual ~LatticeFunction() {}

  std::string name() const { return "LatticeFunction"; }

  void functionLattice(const API::LatticeDomain &latticeDomain,
                       API::FunctionValues &values) const;

  void setCrystalSystem(const std::string &crystalSystem);
  void setUnitCell(const std::string &unitCellString);

protected:
  void init();
  void beforeDecoratedFunctionSet(const API::IFunction_sptr &fn);

private:
  PawleyParameterFunction_sptr m_cellParameters;
};

typedef boost::shared_ptr<LatticeFunction> LatticeFunction_sptr;

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_LATTICEFUNCTION_H_ */
