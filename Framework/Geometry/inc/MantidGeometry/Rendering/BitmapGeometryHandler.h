#ifndef BITMAPGEOMETRYHANDLER_H
#define BITMAPGEOMETRYHANDLER_H

#ifndef Q_MOC_RUN
#include <boost/weak_ptr.hpp>
#endif
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/Object.h"
namespace Mantid {

namespace Geometry {
/**
\class BitmapGeometryHandler
\brief Handler for geometry objects that are rendered as bitmaps (e.g.
RectangularDetector), rather than primitives.
\author Janik Zikovsky
\date October 2010
\version 1.0

This class supports drawing RectangularDetector - as a bitmap plotted by openGL
rather
than a million individual pixels rendered as cubes.
A texture will have been created by the RectangularDetectorActor (in MantidPlot)
and this is what will be mapped.

Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class ObjComponent;
class Object;
class MANTID_GEOMETRY_DLL BitmapGeometryHandler : public GeometryHandler {
private:
  static Kernel::Logger &PLog; ///< The official logger

  boost::shared_ptr<GeometryHandler> clone() const;

  /// The RectangularDetector object being plotted.
  RectangularDetector *m_rectDet;

public:
  BitmapGeometryHandler(RectangularDetector *comp);
  BitmapGeometryHandler();
  //
  //                      BitmapGeometryHandler(IObjComponent *comp);   ///<
  //                      Constructor
  //                      BitmapGeometryHandler(boost::shared_ptr<Object> obj);
  //                      ///<Constructor
  //                      BitmapGeometryHandler(Object *obj); ///<Constructor
  virtual ~BitmapGeometryHandler();
  virtual BitmapGeometryHandler *
  createInstance(IObjComponent *); ///< Create an instance of concrete geometry
  /// handler for ObjComponent
  virtual BitmapGeometryHandler *createInstance(boost::shared_ptr<
      Object>); ///< Create an instance of concrete geometry handler for Object
  virtual GeometryHandler *createInstance(
      Object *); ///< Create an instance of concrete geometry handler for Object
  virtual void Triangulate(); ///< Triangulate the Object
  virtual void Render();      ///< Render Object or ObjComponent
  virtual void
  Initialize(); ///< Prepare/Initialize Object/ObjComponent to be rendered
  /// Returns true if the shape can be triangulated
  virtual bool canTriangulate() { return false; }
  /// get the number of triangles
  virtual int NumberOfTriangles() { return 0; }
  /// get the number of points or vertices
  virtual int NumberOfPoints() { return 0; }
  /// Extract the vertices of the triangles
  virtual double *getTriangleVertices() { return NULL; }
  /// Extract the Faces of the triangles
  virtual int *getTriangleFaces() { return NULL; }
  /// Sets the geometry cache using the triangulation information provided
  virtual void setGeometryCache(int noPts, int noFaces, double *pts,
                                int *faces) {
    (void)noPts;
    (void)noFaces;
    (void)pts;
    (void)faces; // Avoid compiler warning
  };
  /// return the actual type and points of one of the "standard" objects,
  /// cuboid/cone/cyl/sphere
  virtual void GetObjectGeom(int &mytype, std::vector<Kernel::V3D> &vectors,
                             double &myradius, double &myheight) {
    (void)mytype;
    (void)vectors;
    (void)myradius;
    (void)myheight; // Avoid compiler warning
  };
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif
