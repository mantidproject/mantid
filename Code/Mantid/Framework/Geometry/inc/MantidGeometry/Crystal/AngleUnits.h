#ifndef MANTID_GEOMETRY_ANGLEUNITS_H_
#define MANTID_GEOMETRY_ANGLEUNITS_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/PhysicalConstants.h"
#include <cmath>

namespace Mantid
{
  namespace Geometry
  {
    /**
      Defines units/enum for Crystal work

      Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

      File change history is stored at: <https://github.com/mantidproject/mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
     */


    /// Degrees to radians conversion factor
    const double deg2rad=M_PI/180.;

    /// Radians to degrees conversion factor
    const double rad2deg=180./M_PI;

    /// Flag for angle units used in UnitCell class
    enum AngleUnits { angDegrees, angRadians};

  }
}


#endif /* MANTID_GEOMETRY_ANGLEUNITS_H_ */
