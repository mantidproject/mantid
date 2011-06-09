/*
 * Coordinate.h
 *
 *  Created on: 25 Feb 2011
 *      Author: owen
 */

#ifndef MD_Coordinate_H_
#define MD_Coordinate_H_

/** The class represents a 4dimensional Coordinate. Supports simple rendering of MDCells and MDPoints.
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
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <vector>

namespace Mantid
{
 ///** Typedef for the data type to use for the signal and error
 //  * integrated in MDWorkspaces, MDBoxes, MDEventWorkspace etc.
 //  *
 //  * This could be a float or a double, depending on requirements/platform.
 //  * We can change this in order to compare performance/memory/accuracy requirements.
 //  */
 //typedef double coord_t;
 //typedef double signal_t;

namespace Geometry
{
  

class MANTID_GEOMETRY_DLL Coordinate
{
public:
  ///Default constructor. All vertexes zero.
  Coordinate();

  /// Construct from an array
  Coordinate(Mantid::coord_t * coords, size_t numdims);

  //Copy constructor
  Coordinate(const Coordinate & other);

  //Assignment operator
  Coordinate & operator= (const Coordinate & other);

  ///Construction Method for 1D.
  static Coordinate createCoordinate1D(const coord_t& xArg);

  ///Construction Method for 2D.
  static Coordinate createCoordinate2D(const coord_t& xArg, const coord_t& yArg);

  ///Construction Method for 3D.
  static Coordinate createCoordinate3D(const coord_t& xArg, const coord_t& yArg, const coord_t& zArg);

  ///Construction Method for 4D.
  static Coordinate createCoordinate4D(const coord_t& xArg, const coord_t& yArg,const coord_t& zArg,const coord_t& tArg);

  /// Getter for x value
  coord_t getX() const;

  /// Getter for y value.
  coord_t getY() const;

  /// Getter for z value.
  coord_t getZ() const;

  /// Getter for t value.
  coord_t gett() const;

private:

  coord_t m_x;
  coord_t m_y;
  coord_t m_z;
  coord_t m_t;

  ///Constructor for 1D.
  Coordinate(const coord_t& xArg);

  ///Constructor for 2D.
  Coordinate(const coord_t& xArg, const coord_t& yArg);

  ///Constructor for 3D.
  Coordinate(const coord_t& xArg, const coord_t& yArg, const coord_t& zArg);

  ///Constructor for 4D.
  Coordinate(const coord_t& xArg, const coord_t& yArg,const coord_t& zArg,const coord_t& tArg);
};

typedef std::vector<Coordinate> VecCoordinate;
}
}

#endif /* Coordinate_H_ */
