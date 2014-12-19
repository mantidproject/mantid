#ifndef IFUNCTONBUILDER_H_
#define IFUNCTONBUILDER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "ImplicitFunctionParameter.h"

namespace Mantid {

namespace API {
/** This class is the abstract type for building IImplicitFunctions

@author Owen Arnold, Tessella plc
@date 01/10/2010

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MANTID_API_DLL ImplicitFunctionBuilder {
public:
  virtual Mantid::Geometry::MDImplicitFunction *create() const = 0;
  virtual ~ImplicitFunctionBuilder() {}
};
}
}

#endif
