#ifndef MANTID_GEOMETRY_SHAPEFACTORY_H_
#define MANTID_GEOMETRY_SHAPEFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <map>

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
/// @cond Exclude from doxygen documentation
namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco
/// @endcond

namespace Mantid {

namespace Geometry {
class Surface;
class IObject;
class CSGObject;

struct CuboidCorners {
  Kernel::V3D lfb;
  Kernel::V3D lft;
  Kernel::V3D lbb;
  Kernel::V3D rfb;
};

struct Hexahedron {
  Kernel::V3D lfb; // left front bottom
  Kernel::V3D lft; // left front top
  Kernel::V3D lbb; // left back bottom
  Kernel::V3D lbt; // left back top
  Kernel::V3D rfb; // right front bottom
  Kernel::V3D rft; // right front top
  Kernel::V3D rbb; // right back bottom
  Kernel::V3D rbt; // right back top
};
/**

Class originally intended to be used with the DataHandling 'LoadInstrument'
algorithm.
In that algorithm it is used for creating shared pointers to the geometric
shapes
described in the XML instrument definition file.

This class is now also use elsewhere, and in addition to create geometric shapes
from an DOM-element-node pointing to a \<type> element with shape information,
shapes
can also be created directly from a XML shape string.

@author Anders Markvardsen, ISIS, RAL
@date 6/8/2008

Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class MANTID_GEOMETRY_DLL ShapeFactory {
public:
  boost::shared_ptr<CSGObject> createShape(Poco::XML::Element *pElem);
  boost::shared_ptr<CSGObject> createShape(std::string shapeXML,
                                           bool addTypeTag = true);

  boost::shared_ptr<CSGObject> createHexahedralShape(double xlb, double xlf,
                                                     double xrf, double xrb,
                                                     double ylb, double ylf,
                                                     double yrf, double yrb);

private:
  std::string parseSphere(Poco::XML::Element *pElem,
                          std::map<int, boost::shared_ptr<Surface>> &prim,
                          int &l_id);
  std::string
  parseInfinitePlane(Poco::XML::Element *pElem,
                     std::map<int, boost::shared_ptr<Surface>> &prim,
                     int &l_id);
  std::string
  parseInfiniteCylinder(Poco::XML::Element *pElem,
                        std::map<int, boost::shared_ptr<Surface>> &prim,
                        int &l_id);
  std::string parseCylinder(Poco::XML::Element *pElem,
                            std::map<int, boost::shared_ptr<Surface>> &prim,
                            int &l_id);
  std::string
  parseSegmentedCylinder(Poco::XML::Element *pElem,
                         std::map<int, boost::shared_ptr<Surface>> &prim,
                         int &l_id);
  std::string
  parseHollowCylinder(Poco::XML::Element *pElem,
                      std::map<int, boost::shared_ptr<Surface>> &prim,
                      int &l_id);

  CuboidCorners parseCuboid(Poco::XML::Element *pElem);
  std::string parseCuboid(Poco::XML::Element *pElem,
                          std::map<int, boost::shared_ptr<Surface>> &prim,
                          int &l_id);
  std::string parseInfiniteCone(Poco::XML::Element *pElem,
                                std::map<int, boost::shared_ptr<Surface>> &prim,
                                int &l_id);
  std::string parseCone(Poco::XML::Element *pElem,
                        std::map<int, boost::shared_ptr<Surface>> &prim,
                        int &l_id);

  static std::string
  parseHexahedronFromStruct(Hexahedron &hex,
                            std::map<int, boost::shared_ptr<Surface>> &prim,
                            int &l_id);
  Hexahedron parseHexahedron(Poco::XML::Element *pElem);
  std::string parseHexahedron(Poco::XML::Element *pElem,
                              std::map<int, boost::shared_ptr<Surface>> &prim,
                              int &l_id);
  std::string parseTaperedGuide(Poco::XML::Element *pElem,
                                std::map<int, boost::shared_ptr<Surface>> &prim,
                                int &l_id);
  std::string parseTorus(Poco::XML::Element *pElem,
                         std::map<int, boost::shared_ptr<Surface>> &prim,
                         int &l_id);
  std::string
  parseSliceOfCylinderRing(Poco::XML::Element *pElem,
                           std::map<int, boost::shared_ptr<Surface>> &prim,
                           int &l_id);

  Poco::XML::Element *getShapeElement(Poco::XML::Element *pElem,
                                      const std::string &name);
  Poco::XML::Element *getOptionalShapeElement(Poco::XML::Element *pElem,
                                              const std::string &name);
  double getDoubleAttribute(Poco::XML::Element *pElem, const std::string &name);
  Kernel::V3D parsePosition(Poco::XML::Element *pElem);
  void createGeometryHandler(Poco::XML::Element *,
                             boost::shared_ptr<CSGObject>);
};

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_SHAPEFACTORY_H_*/
