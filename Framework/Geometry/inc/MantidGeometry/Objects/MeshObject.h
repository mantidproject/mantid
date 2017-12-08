#ifndef MANTID_GEOMETRY_MESHOBJECT_H_
#define MANTID_GEOMETRY_MESHOBJECT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/IObject.h"

#include "BoundingBox.h"
#include <map>
#include <memory>

namespace Mantid {
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel {
class PseudoRandomNumberGenerator;
class Material;
class V3D;
}

namespace Geometry {
class CacheGeometryHandler;
class CompGrp;
class GeometryHandler;
class Track;
class vtkGeometryCacheReader;
class vtkGeometryCacheWriter;

/**
\class MeshObject
\brief Triangular Mesh Object
\version 1.0
\date Dec 2017
\author S. Ansell

Mesh Object of Traingles

Copyright &copy; 2017-2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL MeshObject : public IObject {
public:
  /// Default constructor
  MeshObject();
  /// Constructor from string.
  MeshObject(const std::string &shapeXML);
  /// Copy constructor
  MeshObject(const MeshObject &);
  /// Assignment operator
  MeshObject &operator=(const MeshObject &);
  /// Destructor
  virtual ~MeshObject();
  /// Clone
  IObject *clone() const override { return new MeshObject(*this); }

  void setID(const std::string &id) override { m_id = id; }
  const std::string &id() const override { return m_id; }

  void setName(const int nx) override { m_object_number = nx; } ///< Set Name
  int getName() const override { return m_object_number; }      ///< Get Name

  void setMaterial(const Kernel::Material &material) override;
  const Kernel::Material material() const override;

  /// Return whether this object has a valid shape
  bool hasValidShape() const override;

  bool
  isValid(const Kernel::V3D &) const override; ///< Check if a point is inside
  bool isOnSide(const Kernel::V3D &) const override;
  int calcValidType(const Kernel::V3D &Pt,
                    const Kernel::V3D &uVec) const override;


  // INTERSECTION
  int interceptSurface(Geometry::Track &) const override;

  // Solid angle - uses triangleSolidAngle unless many (>30000) triangles
  double solidAngle(const Kernel::V3D &observer) const override;
  // Solid angle with a scaling of the object
  double solidAngle(const Kernel::V3D &observer,
                    const Kernel::V3D &scaleFactor) const override;

  /// Calculates the volume of this object.
  double volume() const override;

  /// Calculate (or return cached value of) Axis Aligned Bounding box
  /// (DEPRECATED)
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin) const override;

  /// Return cached value of axis-aligned bounding box
  const BoundingBox &getBoundingBox() const override;
  /// Set a null bounding box for this object
  void setNullBoundingBox();

  // find internal point to object
  int getPointInObject(Kernel::V3D &point) const override;

  /// Select a random point within the object
  Kernel::V3D generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                    const size_t) const override;
  Kernel::V3D generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                    const BoundingBox &activeRegion,
                                    const size_t) const override;

  // Rendering member functions
  void draw() const override;
  // Initialize Drawing
  void initDraw() const override;
  // Get Geometry Handler
  boost::shared_ptr<GeometryHandler> getGeometryHandler() override;
  /// Set Geometry Handler
  void setGeometryHandler(boost::shared_ptr<GeometryHandler> h);

  /// set vtkGeometryCache writer
  void setVtkGeometryCacheWriter(
      boost::shared_ptr<vtkGeometryCacheWriter>) override;
  /// set vtkGeometryCache reader
  void setVtkGeometryCacheReader(
      boost::shared_ptr<vtkGeometryCacheReader>) override;
  void GetObjectGeom(int &type, std::vector<Kernel::V3D> &vectors,
                     double &myradius, double &myheight) const override;
  /// Getter for the shape xml
  std::string getShapeXML() const override;

private:
  /// Calculate bounding box using object's vertices
  void calcBoundingBoxByVertices();

  /// Calculate bounding box using object's geometric data
  void calcBoundingBoxByGeometry();

  /// Object's bounding box
  BoundingBox m_boundingBox;
  /// Geometry Handle for rendering
  boost::shared_ptr<GeometryHandler> m_handler;

  /// Numerical identifier of object
  int m_object_number;

  /// Optional string identifier
  std::string m_id;
  /// material composition
  std::unique_ptr<Kernel::Material> m_material;

  void updateGeometryHandler();

  /// a pointer to a class for reading from the geometry cache
  boost::shared_ptr<vtkGeometryCacheReader> m_vtk_cache_reader;
  /// a pointer to a class for writing to the geometry cache
  boost::shared_ptr<vtkGeometryCacheWriter> m_vtk_cache_writer;

  // String from which object may be defined
  std::string m_string;

  /// Read access to mesh object contents
  int numberOfVertices() const;
  double *getVertices() const;
  int numberOfTriangles() const;
  int *getTriangles() const;
};

} // NAMESPACE Geometry
} // NAMESPACE Mantid

#endif /*MANTID_GEOMETRY_MESHOBJECT_H_*/
