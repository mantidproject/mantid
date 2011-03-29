#ifndef MANTID_VATES_GEOMETRYPROXY_H_
#define MANTID_VATES_GEOMETRYPROXY_H_

#include "MantidVatesAPI/Common.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidKernel/System.h"
#include "MDDataObjects/MDImage.h"
#include "MDDataObjects/MDWorkspace.h"
#include <boost/scoped_ptr.hpp>
#include <map>

namespace Mantid
{
namespace VATES
{

/** Proxy for geometry. Allows the dimension related data to be fetched from an underlying geometry object in a runtime flexible fashion.
 * Ultimately reduces the need for rebinning operations where dimensions are simply remapped.
 *

@author Owen Arnold, Tessella plc
@date 21/03/2011

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

class DLLExport GeometryProxy
{

public:
  /// type definitions for member function returning Dimension_sptr
  //typedef Dimension_sptr (Mantid::Geometry::MDGeometry::*MemFuncGetter)() const;
  typedef Mantid::Geometry::MDDimension_sptr (Mantid::Geometry::MDGeometry::*MemFuncGetter)() const;

  typedef Mantid::Geometry::MDGeometry Geometry;

  /// Find the member function corresonding to the key/dimension id.
  MemFuncGetter find(std::string key) const;

  /// Constructional method.
  static GeometryProxy* New(Mantid::Geometry::MDGeometry* geometry, Dimension_sptr  xDim, Dimension_sptr  yDim,
  Dimension_sptr  zDim, Dimension_sptr  tDim);

  /// Destructor.
  ~GeometryProxy();

  /// Get X Dimension via internal memberfunction-to-dimension id map.
  Dimension_sptr getXDimension(void) const;
  /// Get Y Dimension via internal memberfunction-to-dimension id map.
  Dimension_sptr getYDimension(void) const;
  /// Get Z Dimension via internal memberfunction-to-dimension id map.
  Dimension_sptr getZDimension(void) const;
  /// Get t Dimension
  Dimension_sptr getTDimension(void) const;

  ///Returns a function wrapper bound to the the MDImage getPoint method, which is rebound depending upon the dimension arrangement.
  boost::function<Mantid::MDDataObjects::MD_image_point(int, int, int, int)> getMappedPointFunction(
      Mantid::MDDataObjects::MDImage_sptr image);

private:

  /// Constructor.
  GeometryProxy(Mantid::Geometry::MDGeometry* geometry, Dimension_sptr  xDim, Dimension_sptr  yDim,
        Dimension_sptr  zDim, Dimension_sptr  tDim);

  /// Separate initalizer as may throw.
  void initalize();

  /// Wrapped geometry for this proxy.
  Mantid::Geometry::MDGeometry* m_geometry;
  /// Actual x dimension
  Dimension_sptr m_xDimension;
  /// Actual y dimension
  Dimension_sptr m_yDimension;
  /// Actual z dimension
  Dimension_sptr m_zDimension;
  /// Actual t dimension
  Dimension_sptr m_tDimension;
  /// map of id to member function.
  std::map<std::string, MemFuncGetter> m_fmap;

  GeometryProxy(const GeometryProxy&); // Not implemented.
  void operator=(const GeometryProxy&); // Not implemented.

};

/// Helper function used to determine if a dimension is qx dimension
DLLExport bool isQxDimension(Mantid::Geometry::IMDDimension_sptr dimension);

/// Helper function used to determine if a dimension is qy dimension
DLLExport bool isQyDimension(Mantid::Geometry::IMDDimension_sptr dimension);

/// Helper function used to determine if a dimension is qz dimension
DLLExport bool isQzDimension(Mantid::Geometry::IMDDimension_sptr dimension);

}
}

#endif
