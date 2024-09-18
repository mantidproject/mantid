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
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"

#include <map>
#include <memory>
#include <optional>

namespace Mantid {
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel {
class PseudoRandomNumberGenerator;
class Material;
class V3D;
} // namespace Kernel

namespace Geometry {
class CompGrp;
class GeometryHandler;
class Rule;
class Surface;
class Track;
class vtkGeometryCacheReader;
class vtkGeometryCacheWriter;

/**
\class CSGObject
\brief Constructive Solid Geometry object
\version 1.0
\date July 2007
\author S. Ansell

A Constructive Solid Geometry (CSG) object, implemented
as a collection of Rules and surface objects
*/
class MANTID_GEOMETRY_DLL CSGObject final : public IObject {
public:
  /// Default constructor
  CSGObject();
  /// Constructor providing shape xml.
  CSGObject(std::string shapeXML);
  /// Copy constructor
  CSGObject(const CSGObject &);
  /// Assignment operator
  CSGObject &operator=(const CSGObject &);
  /// Destructor
  ~CSGObject() override;
  /// Clone
  IObject *clone() const override { return new CSGObject(*this); }

  IObject *cloneWithMaterial(const Kernel::Material &material) const override {
    auto obj = new CSGObject(*this);
    obj->setMaterial(material);
    return obj;
  }

  bool isFiniteGeometry() const override { return m_isFiniteGeometry; }
  void setFiniteGeometryFlag(bool isFinite) override { m_isFiniteGeometry = isFinite; }

  /// Return the top rule
  const Rule *topRule() const { return m_topRule.get(); }
  void setID(const std::string &id) override { m_id = id; }
  const std::string &id() const override { return m_id; }

  void setName(const int objNum) { m_objNum = objNum; } ///< Set Name
  int getName() const override { return m_objNum; }     ///< Get Name

  void setMaterial(const Kernel::Material &material) override;
  const Kernel::Material &material() const override;

  /// Return whether this object has a valid shape
  bool hasValidShape() const override;
  int setObject(const int objName, const std::string &lineStr);
  int procString(const std::string &lineStr);
  int complementaryObject(const int cellNum,
                          std::string &lineStr); ///< Process a complementary object
  int hasComplement() const;

  int populate(const std::map<int, std::shared_ptr<Surface>> &);
  int createSurfaceList(const int outFlag = 0); ///< create Surface list
  int addSurfString(const std::string &);       ///< Not implemented
  int removeSurface(const int surfNum);
  int substituteSurf(const int surfNum, const int newSurfNum, const std::shared_ptr<Surface> &surfPtr);
  void makeComplement();
  void convertComplement(const std::map<int, CSGObject> &);

  virtual void print() const;
  void printTree() const;

  bool isValid(const Kernel::V3D &) const override; ///< Check if a point is valid
  bool isValid(const std::map<int, int> &) const;   ///< Check if a set of surfaces are valid.
  bool isOnSide(const Kernel::V3D &) const override;
  Mantid::Geometry::TrackDirection calcValidType(const Kernel::V3D &Pt, const Kernel::V3D &uVec) const;
  Mantid::Geometry::TrackDirection calcValidTypeBy3Points(const Kernel::V3D &prePt, const Kernel::V3D &curPt,
                                                          const Kernel::V3D &nxtPt) const;

  std::vector<int> getSurfaceIndex() const;
  /// Get the list of surfaces (const version)
  const std::vector<const Surface *> &getSurfacePtr() const { return m_surList; }
  /// Get the list of surfaces
  std::vector<const Surface *> &getSurfacePtr() { return m_surList; }

  std::string cellCompStr() const;
  std::string cellStr(const std::map<int, CSGObject> &) const;

  std::string str() const;
  void write(std::ostream &) const; ///< MCNPX output

  // INTERSECTION
  int interceptSurface(Geometry::Track &track) const override;
  double distance(const Track &track) const override;

  // Solid angle - uses triangleSolidAngle unless many (>30000) triangles
  double solidAngle(const SolidAngleParams &params) const override;
  // Solid angle with a scaling of the object
  double solidAngle(const SolidAngleParams &params, const Kernel::V3D &scaleFactor) const override;
  // solid angle via triangulation
  double triangulatedSolidAngle(const SolidAngleParams &params) const;
  // Solid angle via triangulation with scaling factor for object size
  double triangulatedSolidAngle(const SolidAngleParams &params, const Kernel::V3D &scaleFactor) const;
  // solid angle via ray tracing
  double rayTraceSolidAngle(const Kernel::V3D &observer) const;

  /// Calculates the volume of this object.
  double volume() const override;

  /// Calculate (or return cached value of) Axis Aligned Bounding box
  /// (DEPRECATED)
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin,
                      double &zmin) const override;

  /// Return cached value of axis-aligned bounding box
  const BoundingBox &getBoundingBox() const override;
  /// Define axis-aligned bounding box
  void defineBoundingBox(const double &xMax, const double &yMax, const double &zMax, const double &xMin,
                         const double &yMin, const double &zMin);
  /// Set a null bounding box for this object
  void setNullBoundingBox();
  // find internal point to object
  int getPointInObject(Kernel::V3D &point) const override;

  /// Select a random point within the object
  std::optional<Kernel::V3D> generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                                   const size_t) const override;
  std::optional<Kernel::V3D> generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                                   const BoundingBox &activeRegion, const size_t) const override;

  // Rendering member functions
  void draw() const override;
  // Initialize Drawing
  void initDraw() const override;
  // Get Geometry Handler
  std::shared_ptr<GeometryHandler> getGeometryHandler() const override;
  /// Set Geometry Handler
  void setGeometryHandler(const std::shared_ptr<GeometryHandler> &h);

  /// set vtkGeometryCache writer
  void setVtkGeometryCacheWriter(std::shared_ptr<vtkGeometryCacheWriter>);
  /// set vtkGeometryCache reader
  void setVtkGeometryCacheReader(std::shared_ptr<vtkGeometryCacheReader>);
  detail::ShapeInfo::GeometryShape shape() const override;
  const detail::ShapeInfo &shapeInfo() const override;
  void GetObjectGeom(detail::ShapeInfo::GeometryShape &type, std::vector<Kernel::V3D> &vectors, double &innerRadius,
                     double &radius, double &height) const override;
  /// Getter for the shape xml
  std::string getShapeXML() const;

private:
  int procPair(std::string &lineStr, std::map<int, std::unique_ptr<Rule>> &ruleMap, int &compUnit) const;
  std::unique_ptr<CompGrp> procComp(std::unique_ptr<Rule>) const;
  int checkSurfaceValid(const Kernel::V3D &, const Kernel::V3D &) const;

  /// Calculate bounding box using Rule system
  void calcBoundingBoxByRule();

  /// Calculate bounding box using object's vertices
  void calcBoundingBoxByVertices();

  /// Calculate bounding box using object's geometric data
  void calcBoundingBoxByGeometry();

  int searchForObject(Kernel::V3D &) const;

  /// Returns the volume.
  double monteCarloVolume() const;
  /// Returns the volume.
  double singleShotMonteCarloVolume(const int shotSize, const size_t seed) const;
  /// Top rule [ Geometric scope of object]
  std::unique_ptr<Rule> m_topRule;
  /// Object's bounding box
  BoundingBox m_boundingBox;
  // -- DEPRECATED --
  mutable double AABBxMax,  ///< xmax of Axis aligned bounding box cache
      AABByMax,             ///< ymax of Axis aligned bounding box cache
      AABBzMax,             ///< zmax of Axis aligned bounding box cache
      AABBxMin,             ///< xmin of Axis aligned bounding box cache
      AABByMin,             ///< xmin of Axis aligned bounding box cache
      AABBzMin;             ///< zmin of Axis Aligned Bounding Box Cache
  mutable bool boolBounded; ///< flag true if a bounding box exists, either by

  /// Creation number
  int m_objNum;
  /// Geometry Handle for rendering
  std::shared_ptr<GeometryHandler> m_handler;
  friend class GeometryHandler;
  friend class GeometryRenderer;
  /// Is geometry caching enabled?
  bool bGeometryCaching;
  /// a pointer to a class for reading from the geometry cache
  std::shared_ptr<vtkGeometryCacheReader> vtkCacheReader;
  /// a pointer to a class for writing to the geometry cache
  std::shared_ptr<vtkGeometryCacheWriter> vtkCacheWriter;
  void updateGeometryHandler();
  size_t numberOfTriangles() const;
  size_t numberOfVertices() const;
  /// for solid angle from triangulation
  const std::vector<uint32_t> &getTriangleFaces() const;
  const std::vector<double> &getTriangleVertices() const;
  /// original shape xml used to generate this object.
  std::string m_shapeXML;
  /// Optional string identifier
  std::string m_id;
  /// material composition
  mutable std::unique_ptr<Kernel::Material> m_material;
  /// Whether or not the object geometry is finite
  bool m_isFiniteGeometry = true;

protected:
  std::vector<const Surface *> m_surList; ///< Full surfaces (make a map
  /// including complementary object ?)
};

} // NAMESPACE Geometry
} // NAMESPACE Mantid
