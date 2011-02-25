/*
 * Coordinate.h
 *
 *  Created on: 25 Feb 2011
 *      Author: owen
 */

#ifndef MD_COORDINATE_H_
#define MD_COORDINATE_H_

/** The class represents a 4dimensional coordinate. Supports simple rendering of MDCells and MDPoints.
*
*   Abstract type for a multi dimensional dimension. Gives a read-only layer to the concrete implementation.

    @author Owen Arnold, RAL ISIS
    @date 25/02/2011

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
#include "MantidGeometry/DllExport.h"

#include <vector>

namespace Mantid
{
namespace Geometry
{
class EXPORT_OPT_MANTID_GEOMETRY coordinate
{
public:
  ///Default constructor. All vertexes zero.
  coordinate();

  //Copy constructor
  coordinate(const coordinate & other);

  //Assignment operator
  coordinate & operator= (const coordinate & other);

  ///Construction Method for 1D.
  static coordinate createCoordinate1D(const double& xArg);

  ///Construction Method for 2D.
  static coordinate createCoordinate2D(const double& xArg, const double& yArg);

  ///Construction Method for 3D.
  static coordinate createCoordinate3D(const double& xArg, const double& yArg, const double& zArg);

  ///Construction Method for 4D.
  static coordinate createCoordinate4D(const double& xArg, const double& yArg,const double& zArg,const double& tArg);

  /// Getter for x value
  double getX() const;

  /// Getter for y value.
  double getY() const;

  /// Getter for z value.
  double getZ() const;

  /// Getter for t value.
  double gett() const;

private:

  double m_x;
  double m_y;
  double m_z;
  double m_t;

  ///Constructor for 1D.
  coordinate(const double& xArg);

  ///Constructor for 2D.
  coordinate(const double& xArg, const double& yArg);

  ///Constructor for 3D.
  coordinate(const double& xArg, const double& yArg, const double& zArg);

  ///Constructor for 4D.
  coordinate(const double& xArg, const double& yArg,const double& zArg,const double& tArg);
};

typedef std::vector<coordinate> VecCoordinate;
}
}

#endif /* COORDINATE_H_ */
