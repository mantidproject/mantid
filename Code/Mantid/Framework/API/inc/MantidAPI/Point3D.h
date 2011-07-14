#ifndef _POINT_3D_ABSTRACT_H
#define  _POINT_3D_ABSTRACT_H

namespace Mantid
{

    namespace API
    {
  

        /** 
        Abstract type for point data.

        @author Owen Arnold, Tessella plc
        @date 27/10/2010

        Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

        class MANTID_API_DLL Point3D
        {
        public:
          /// Default constructor
          Point3D() : x(0), y(0), z(0)
          {}

          /// Constructor with 3 points
          Point3D(double x, double y, double z) : x(x), y(y), z(z)
          { }

          /// Return the X value
          virtual double getX() const { return x; }
          /// Return the Y value
          virtual double getY() const { return y; }
          /// Return the Z value
          virtual double getZ() const { return z; }
          virtual ~Point3D(){};
        protected:
          /// coordinate
          double x;
          /// coordinate
          double y;
          /// coordinate
          double z;
        };

    }
}

#endif
