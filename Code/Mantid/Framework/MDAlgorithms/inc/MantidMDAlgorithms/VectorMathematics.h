#ifndef MANTID_ALGORITHMS_VECTORMATHEMATICS
#define MANTID_ALGORITHMS_VECTORMATHEMATICS

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <cmath>
#include "MantidGeometry/V3D.h"

namespace Mantid
{
namespace MDAlgorithms
{
/**

 Grouping of static methods used to perform vector mathematics required for MDAlgorithm support.

 Convenience functions wrap V3D mathematical functions without the need to create temporaries.

 @author Owen Arnold, Tessella plc
 @date 19/10/2010

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
//TODO: consider replacing with something more Mantid generic.


double dotProduct(Mantid::Geometry::V3D a, Mantid::Geometry::V3D b);

double dotProduct(double a1, double a2, double a3, double b1, double b2, double b3);

Mantid::Geometry::V3D crossProduct(Mantid::Geometry::V3D a, Mantid::Geometry::V3D b);

Mantid::Geometry::V3D crossProduct(double a1, double a2, double a3, double b1, double b2,
    double b3);

double absolute(double a1, double a2, double a3);

}
}


#endif
