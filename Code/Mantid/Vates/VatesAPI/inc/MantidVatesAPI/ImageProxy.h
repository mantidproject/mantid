#ifndef MANTID_VATES_IMAGEPROCESSOR_H_
#define MANTID_VATES_IMAGEPROCESSOR_H_

#include "MantidKernel/System.h"
#include "MDDataObjects/MDImage.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidVatesAPI/GeometryProxy.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
namespace VATES
{

/** Proxy for a md image. Uses a geometry proxy to re-wire calls to getPoint in a manner that allow runtime flexiblity in the argument order.

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

class DLLExport ImageProxy
{
private:

  /// Assisting mdgeometry proxy. Constructs and contains all remapping information.
  boost::scoped_ptr<GeometryProxy> m_geometryProxy;

  /// underlying MDImage. The subject of this proxy.
  Mantid::MDDataObjects::MDImage_sptr m_image;

  /// Cached member function provided by geometryProxy.
  boost::function<Mantid::MDDataObjects::MD_image_point(int, int, int, int)> m_Function;

  /// Constructor.
  ImageProxy(GeometryProxy* geometryProxy, Mantid::MDDataObjects::MDImage_sptr image);

  /// Setup method. may throw.
  void initalize();

  ImageProxy(const ImageProxy&); // Not implemented.
  void operator=(const ImageProxy&); // Not implemented.

public:

  /// Constructional method.
  static ImageProxy* New(GeometryProxy* geometryProxy, Mantid::MDDataObjects::MDImage_sptr image);

  /// Destructor
 ~ImageProxy();

  /// Embedded type information. Used for static polymorphism.
  typedef GeometryProxy GeometryType;

  /// Method from MDImage. Statically overriden here.
  GeometryProxy * const getGeometry();

  /// Method from MDImage. Statically overriden here.
  Mantid::MDDataObjects::MD_image_point getPoint(int i, int j, int k, int t) const;



};

}
}

#endif
