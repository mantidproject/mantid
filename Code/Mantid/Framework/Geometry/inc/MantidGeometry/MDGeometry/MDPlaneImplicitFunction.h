#ifndef MANTID_MDGEOMETRY_MDPLANEIMPLICITFUNCTION_H_
#define MANTID_MDGEOMETRY_MDPLANEIMPLICITFUNCTION_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"

namespace Mantid
{
namespace Geometry
{

/** A general N-dimensional plane implicit function.
  This relies on MDPlane to do the heavy lifting. The main thing for
  this class is to be able to specify itself via XML.

  @date 2011-13-12

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport MDPlaneImplicitFunction : public MDImplicitFunction
{
public:
    MDPlaneImplicitFunction();
    virtual ~MDPlaneImplicitFunction();
};

}
}

#endif // MANTID_MDGEOMETRY_MDPLANEIMPLICITFUNCTION_H_
