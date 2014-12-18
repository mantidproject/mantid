#ifndef GEOMETRY_BASEVISIT_H
#define GEOMETRY_BASEVISIT_H

#include "MantidGeometry/DllConfig.h"

namespace Mantid
{
  namespace Geometry
  {

    class Surface;
    class Quadratic;
    class Plane;
    class Cylinder;
    class Cone;
    class Sphere;
    class General;
    class Line;

    /**
    \class BaseVisit
    \version 1.0
    \author S. Ansell
    \brief Adds the main

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    */

    class MANTID_GEOMETRY_DLL BaseVisit
    {

    public:

      /// Destructor
      virtual ~BaseVisit() {}

      virtual void Accept(const Surface&) =0;  ///< Accept a surface
      virtual void Accept(const Plane&) =0;    ///< Accept a plane
      virtual void Accept(const Sphere&) =0;   ///< Accept a sphere
      virtual void Accept(const Cone&) =0;     ///< Accept a cone
      virtual void Accept(const Cylinder&) =0; ///< Accept a cylinder
      virtual void Accept(const General&) =0;  ///< Accept a general surface

    };

  } // NAMESPACE Geometry
} // NAMESPACE Mantid

#endif
