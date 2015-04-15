#ifndef MANTID_API_ILATTICEFUNCTION_H_
#define MANTID_API_ILATTICEFUNCTION_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/LatticeDomain.h"
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid {
namespace API {

/** ILatticeFunction

  This abstract class defines the interface for a function that

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
class MANTID_API_DLL ILatticeFunction : public FunctionParameterDecorator {
public:
  ILatticeFunction();
  virtual ~ILatticeFunction() {}

  void function(const FunctionDomain &domain, FunctionValues &values) const;
  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian);

  /// Function that should calculate d-values for the HKLs provided in the
  /// domain.
  virtual void functionLattice(const LatticeDomain &latticeDomain,
                               FunctionValues &values) const = 0;

  virtual void functionDerivLattice(const LatticeDomain &latticeDomain,
                                    Jacobian &jacobian);

  /// A string that names the crystal system.
  virtual void setCrystalSystem(const std::string &crystalSystem) = 0;

  /// Set the function parameters according to the supplied unit cell.
  virtual void setUnitCell(const std::string &unitCellString) = 0;
};

typedef boost::shared_ptr<ILatticeFunction> ILatticeFunction_sptr;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_ILATTICEFUNCTION_H_ */
