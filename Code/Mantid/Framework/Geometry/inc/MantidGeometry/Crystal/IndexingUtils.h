/* File: Indexing_Utils.h */

#ifndef MANTID_GEOMETRY_INDEXING_UTILS_H_
#define MANTID_GEOMETRY_INDEXING_UTILS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Math/Matrix.h"

namespace Mantid
{
namespace Geometry
{
/**
    @class IndexingUtils
  
    This class contains static utility methods for indexing peaks and
    finding the UB matrix.  Currently there is only one method, BestFit_UB
    that finds the best UB matrix given some indexed peaks.
  
    @author Dennis Mikkelson 
    @date   2011-06-14 
     
    Copyright © 2011 ORNL, STFC Rutherford Appleton Laboratories
  
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
    
    File change history is stored at: 
                 <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>

    Code Documentation is available at 
                 <http://doxygen.mantidproject.org>
 */

class MANTID_GEOMETRY_DLL IndexingUtils
{
  public:

  /// Find the UB matrix that best fits 3 or more indexed peaks
  static double BestFit_UB(      Matrix<double>    & UB,
                           const std::vector<V3D>  & hkl_vectors, 
                           const std::vector<V3D>  & q_vectors   );
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_INDEXING_UTILS_H_ */

