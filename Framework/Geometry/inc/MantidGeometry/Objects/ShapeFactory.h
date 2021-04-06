// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#ifndef Q_MOC_RUN
#include <memory>
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
*/
class MANTID_GEOMETRY_DLL ShapeFactory {
public:
  std::shared_ptr<CSGObject> createShape(Poco::XML::Element *pElem);
  std::shared_ptr<CSGObject> createShape(std::string shapeXML, bool addTypeTag = true);

  static std::shared_ptr<CSGObject> createSphere(const Kernel::V3D &centre, double radius);
  static std::shared_ptr<CSGObject> createHexahedralShape(double xlb, double xlf, double xrf, double xrb, double ylb,
                                                          double ylf, double yrf, double yrb);

private:
  static std::string sphereAlgebra(const int surfaceID);
  std::string parseSphere(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim, int &l_id);
  std::string parseInfinitePlane(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim, int &l_id);
  std::string parseInfiniteCylinder(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim,
                                    int &l_id);
  std::string parseCylinder(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim, int &l_id);
  std::string parseSegmentedCylinder(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim,
                                     int &l_id);
  std::string parseHollowCylinder(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim, int &l_id);

  CuboidCorners parseCuboid(Poco::XML::Element *pElem);
  std::string parseCuboid(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim, int &l_id);
  std::string parseInfiniteCone(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim, int &l_id);
  std::string parseCone(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim, int &l_id);

  static std::string parseHexahedronFromStruct(Hexahedron &hex, std::map<int, std::shared_ptr<Surface>> &prim,
                                               int &l_id);
  Hexahedron parseHexahedron(Poco::XML::Element *pElem);
  std::string parseHexahedron(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim, int &l_id);
  std::string parseTaperedGuide(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim, int &l_id);
  std::string parseTorus(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim, int &l_id);
  std::string parseSliceOfCylinderRing(Poco::XML::Element *pElem, std::map<int, std::shared_ptr<Surface>> &prim,
                                       int &l_id);

  Poco::XML::Element *getShapeElement(Poco::XML::Element *pElem, const std::string &name);
  Poco::XML::Element *getOptionalShapeElement(Poco::XML::Element *pElem, const std::string &name);
  double getDoubleAttribute(Poco::XML::Element *pElem, const std::string &name);
  Kernel::V3D parsePosition(Poco::XML::Element *pElem);
  void createGeometryHandler(Poco::XML::Element *, std::shared_ptr<CSGObject>);
};

} // namespace Geometry
} // namespace Mantid
