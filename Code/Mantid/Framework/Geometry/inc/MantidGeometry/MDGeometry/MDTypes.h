#ifndef MANTID_GEOMETRY_MDTYPES_H_
#define MANTID_GEOMETRY_MDTYPES_H_

/** 
    This file contains typedefs for entities relating to multi-dimensional
    geometry.
    
    @author Martyn Gigg, Tessella plc

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

#include <limits>

namespace Mantid
{

  /** Typedef for the data type to use for coordinate axes in MD objects such
   * as MDBox, MDEventWorkspace, etc.
   *
   * This could be a float or a double, depending on requirements.
   * We can change this in order to compare
   * performance/memory/accuracy requirements.
   */
  typedef double coord_t;  // moved our of here to the coordinate and the header itself -- from coordinate header to 
  typedef double signal_t; // avoid strange conflict between numeric_limits and <climits> (presumably)
 
  
  /// Minimum value (large negative number) that a coordinate can take
  static const coord_t coord_t_min = -std::numeric_limits<coord_t>::max();

  /// Maximum value (large positive number) that a coordinate can take
  static const coord_t coord_t_max = std::numeric_limits<coord_t>::max();

 
}

#endif //MANTID_GEOMETRY_MDTYPES_H_
