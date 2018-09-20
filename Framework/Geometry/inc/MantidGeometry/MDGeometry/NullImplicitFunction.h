#ifndef MANTID_ALGORITHMS_NULLIMPLICITFUNCTION_H_
#define MANTID_ALGORITHMS_NULLIMPLICITFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include <gsl/gsl_blas.h>
#include <vector>

namespace Mantid {
namespace Geometry {
/**
This class represents a Null Implicit function. This type prevents the
requirement
for Null checks and handles where ImplicitFunctions are used.

@author Owen Arnold, Tessella plc
@date 28/01/2011

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport NullImplicitFunction
    : public Mantid::Geometry::MDImplicitFunction {
public:
  std::string getName() const override;
  std::string toXMLString() const override;
  //----------------------MDImplicit function methods ------------
  bool isPointContained(const coord_t *) override { return true; }
  bool isPointContained(const std::vector<coord_t> &) override { return true; }
  // Unhide base class methods (avoids Intel compiler warning)
  using MDImplicitFunction::isPointContained;
  //---------------------------------------------------------------
  static std::string functionName() { return "NullImplicitFunction"; }
};
} // namespace Geometry
} // namespace Mantid

#endif
