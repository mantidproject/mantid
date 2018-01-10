#include "MantidGeometry/Objects/MeshObject.h"

#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rendering/CacheGeometryHandler.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/GluGeometryHandler.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/LineIntersectVisit.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/RegexStrings.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Tolerance.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/error_of_mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/make_shared.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/ranlux.hpp>
#include <boost/random/uniform_01.hpp>

#include <array>
#include <deque>
#include <iostream>
#include <stack>

namespace Mantid {
namespace Geometry {

using Kernel::Material;
using Kernel::V3D;
using Kernel::Quat;

/**
*  Default constuctor
*/
MeshObject::MeshObject() : MeshObject("") {}

MeshObject::MeshObject(const std::string &string)
  : m_boundingBox(), m_object_number(0), m_handler(),
  m_string(string), m_id(), m_material(),
  m_triangles(), m_vertices()
{
}


/**
* Copy constructor
* @param A :: The object to initialise this copy from
*/
MeshObject::MeshObject(const MeshObject &A) : MeshObject() { *this = A; }

/**
* Assignment operator
* @param A :: Object to copy
* @return *this
*/
MeshObject &MeshObject::operator=(const MeshObject &A) {
  if (this != &A) {

  }
  return *this;
}

/// Destructor in .cpp so we can forward declare Rule class.
MeshObject::~MeshObject() = default;


/*
 * Initialise in CacheGeogmetryRenderer format
 */
void MeshObject::initialize(const int nPts, const int nFaces, const double *points,
  int *faces) {

  std::vector<V3D> vertices;
  for (int i=0; i<nPts; ++i) {
    vertices.push_back(V3D(nPts+(3 * i), nPts+(3 * i + 1), nPts+(3 * i + 2)));
  }

  std::vector<int> triangles;
  for (int i = 0; i < 3*nFaces; ++i) {
    triangles.push_back(nFaces + i);
  }

  initialize(triangles, vertices);

}

/*
 * Initialise with vectors
 */
void MeshObject::initialize(const std::vector<int> &faces, const std::vector<V3D> &vertices){
  if (m_vertices.size() == 0) {
    m_vertices = vertices;
    m_triangles = faces;
  }
  else {
    throw std::runtime_error("MeshObject already initialized");
  }
}

/**
 * @param material The new Material that the object is composed from
 */
void MeshObject::setMaterial(const Kernel::Material &material) {
  m_material = Mantid::Kernel::make_unique<Material>(material);
}

/**
 * @return The Material that the object is composed from
 */
const Kernel::Material MeshObject::material() const {
  if (m_material)
    return *m_material;
  else
    return Material();
}

/**
* Returns whether this object has a valid shape
* @returns True if the entire MeshObject may enclose
* one or more volumes.
*/
bool MeshObject::hasValidShape() const {
  return false;
}



/**
* Determines whether Pt is within the object or on the surface
* @param Pt :: Point to be tested
* @returns 1 if true and 0 if false
*/
bool MeshObject::isValid(const Kernel::V3D &Pt) const {
  return false;
}

/**
* Determines wither Pt is on the surface.
* @param Pt :: Point to check
* @returns 1 if the point is on the surface
*/
bool MeshObject::isOnSide(const Kernel::V3D &Pt) const {

  // Ok everthing failed return 0;
  return false;
}


/**
* Given a track, fill the track with valid section
* @param UT :: Initial track
* @return Number of segments added
*/
int MeshObject::interceptSurface(Geometry::Track &UT) const {

  std::vector<V3D> intesectionPoints;
  std::vector<int> entryExitFlags;

  getIntersections(UT.startPoint(), UT.direction(), intesectionPoints, entryExitFlags);
  if (sizeof(intesectionPoints) == 0) return 0; // Quit if no intersections found

  for (size_t i = 0; i < sizeof(intesectionPoints); ++i) {
      UT.addPoint(entryExitFlags[i], intesectionPoints[i], *this);
  }
  UT.buildLink();

  return sizeof(intesectionPoints);
}

/**
 * Get intersection points and their in out directions on the given ray
 * @param start :: Start point of ray
 * @param direction :: Direction of ray
 * @param intersectionPoints :: Intersection points
 * @param EntryExitFlags :: +1 ray enters -1 ray exits at corresponding point
*/
void MeshObject::getIntersections(const Kernel::V3D &start, const Kernel::V3D &direction,
  std::vector<Kernel::V3D> &intersectionPoints, std::vector<int> &entryExitFlags) const {

  V3D vertex1, vertex2, vertex3;
  for (size_t i = 0; getTriangle(i, vertex1, vertex2, vertex3); i++) {

  }

}

/**
* Get intersection points and their in out directions on the given ray
* @param start :: Start point of ray
* @param direction :: Direction of ray
* @param v1 :: First vertex of triangle
* @param v2 :: Second vertex of triangle
* @param v3 :: Third vertex of triangle
* @param intersection :: Intersection point
* @returns true if there is an intersection
*/
bool MeshObject::rayIntersectsTriangle(const Kernel::V3D &start, const Kernel::V3D &direction,
  const V3D &v1, const V3D &v2, const V3D &v3, V3D &intersection) const {
  // Implements Möller–Trumbore intersection algorithm
  V3D edge1, edge2, h, s, q;
  double a, f, u, v;
  edge1 = v2 - v1;
  edge2 = v3 - v1;
  h = direction.cross_prod(edge2);
  a = edge1.scalar_prod(h);

  const double EPSILON = 0.0000001*edge1.norm();
  if (a > -EPSILON && a < EPSILON)
    return false;  // Ray in or parallel to plane of triangle
  f = 1 / a;
  s = start - v1;
  u = f * (s.scalar_prod(h));
  if (u < 0.0 || u > 1.0)
    return false; // Intesection with plane outside triangle
  q = s.cross_prod(edge1);
  v = f * direction.scalar_prod(q);
  if (v < 0.0 || u + v > 1.0)
    return false; // Intesection with plane outside triangle

  // At this stage we can compute t to find out where the intersection point is on the line.
  double t = f * edge2.scalar_prod(q);
  if (t > EPSILON) // ray intersection
  {
    intersection = start + direction * t;
    return true;
  }
  else // This means that intersection occurs on the line of the ray behind the ray.
    return false;

}

/*
* Get a triangle - useful for iterating over triangles
* @param index :: Index of triangle in MeshObject
* @param v1 :: First vertex of triangle
* @param v2 :: Second vertex of triangle
* @param v3 :: Third vertex of triangle
* @returns true if the specified triangle exists
*/
bool MeshObject::getTriangle(const size_t index, V3D &vertex1, V3D &vertex2, V3D &vertex3) const {
  bool triangleExists = index < sizeof(m_triangles) / 3;
  if (triangleExists) {
    vertex1 = m_vertices[3 * index];
    vertex2 = m_vertices[3 * index + 1];
    vertex3 = m_vertices[3 * index + 2];
  }
  return triangleExists;
}

/**
* Calculate if a point PT is a valid point on the track
* @param Pt :: Point to calculate from.
* @param uVec :: Unit vector of the track
* @retval 0 :: Not valid / double valid
* @retval 1 :: Entry point
* @retval -1 :: Exit Point
*/
int MeshObject::calcValidType(const Kernel::V3D &Pt,
                             const Kernel::V3D &uVec) const {
  const Kernel::V3D shift(uVec * Kernel::Tolerance * 25.0);
  const Kernel::V3D testA(Pt - shift);
  const Kernel::V3D testB(Pt + shift);
  const int flagA = isValid(testA);
  const int flagB = isValid(testB);
  if (!(flagA ^ flagB))
    return 0;
  return (flagA) ? -1 : 1;
}

/**
* Takes input axis aligned bounding box max and min points and calculates the
*bounding box for the
* object and returns them back in max and min points.
*
* @param xmax :: Maximum value for the bounding box in x direction
* @param ymax :: Maximum value for the bounding box in y direction
* @param zmax :: Maximum value for the bounding box in z direction
* @param xmin :: Minimum value for the bounding box in x direction
* @param ymin :: Minimum value for the bounding box in y direction
* @param zmin :: Minimum value for the bounding box in z direction
*/
void MeshObject::getBoundingBox(double &xmax, double &ymax, double &zmax,
  double &xmin, double &ymin, double &zmin) const {
  BoundingBox bb = getBoundingBox();
  xmin = bb.xMin();
  xmax = bb.xMax();
  ymin = bb.yMin();
  ymax = bb.yMax();
  zmin = bb.zMin();
  zmax = bb.zMax();

  return;
}

/**
* Find solid angle of object wrt the observer. 
* @param observer :: point to measure solid angle from
* @return :: estimate of solid angle of object. Accuracy depends on object
* shape.
*/
double MeshObject::solidAngle(const Kernel::V3D &observer) const {

  return 0;
}

/**
* Find solid angle of object wrt the observer with a scaleFactor for the object.
* @param observer :: point to measure solid angle from
* @param scaleFactor :: V3D giving scaling of the object
* @return :: estimate of solid angle of object.
*/
double MeshObject::solidAngle(const Kernel::V3D &observer,
                             const Kernel::V3D &scaleFactor) const

{
  return 0;
}

/**
 * Calculate volume. May be approximate for a complicated shape.
 * @return The volume.
 */
double MeshObject::volume() const {
  return 0;
}

/**
* Returns an axis-aligned bounding box that will fit the shape
* @returns A reference to a bounding box for this shape.
*/
const BoundingBox &MeshObject::getBoundingBox() const {

  if(m_boundingBox.isNull())
    // As MeshObject is immutable, we need only calculate 
    // bounding box, if the cached bounding box is null.
    if (sizeof(m_vertices) > 0) {
      // Initial extents to be overwritten by loop
      constexpr double huge = 1e10;
      double minX, maxX, minY, maxY, minZ, maxZ;
      minX = minY = minZ = huge;
      maxX = maxY = maxZ = -huge;

      // Loop over all vertices and determine minima and maxima on each axis
      for (int i = 0; i < sizeof(m_vertices); ++i) {
        auto vx = m_vertices[i].X();
        auto vy = m_vertices[i].Y();
        auto vz = m_vertices[i].Z();

        minX = std::min(minX, vx);
        maxX = std::max(maxX, vx);
        minY = std::min(minY, vy);
        maxY = std::max(maxY, vy);
        minZ = std::min(minZ, vz);
        maxZ = std::max(maxZ, vz);
      }
      // Cache bounding box, so we do not need to repeat calculation
      m_boundingBox = BoundingBox(minX, maxX, minY, maxY, minZ, maxZ);
    }

  return m_boundingBox;
}

/**
Try to find a point that lies within (or on) the object
@param[out] point :: on exit set to the point value, if found
@return 1 if point found, 0 otherwise
*/
int MeshObject::getPointInObject(Kernel::V3D &point) const {

  return 0;
}

/**
 * Generate a random point within the object. The method simply generates a
 * point within the bounding box and tests if this is a valid point within
 * the object: if so the point is return otherwise a new point is selected.
 * @param rng  A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @param maxAttempts The maximum number of attempts at generating a point
 * @return The generated point
 */
V3D MeshObject::generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                     const size_t maxAttempts) const {
  const auto &bbox = getBoundingBox();
  if (bbox.isNull()) {
    throw std::runtime_error("Object::generatePointInObject() - Invalid "
                             "bounding box. Cannot generate new point.");
  }
  return generatePointInObject(rng, bbox, maxAttempts);
}

/**
 * Generate a random point within the object that is also bound by the
 * activeRegion box.
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @param activeRegion Restrict point generation to this sub-region of the
 * object
 * @param maxAttempts The maximum number of attempts at generating a point
 * @return The newly generated point
 */
V3D MeshObject::generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                     const BoundingBox &activeRegion,
                                     const size_t maxAttempts) const {
  size_t attempts(0);
  while (attempts < maxAttempts) {
    const double r1 = rng.nextValue();
    const double r2 = rng.nextValue();
    const double r3 = rng.nextValue();
    auto pt = activeRegion.generatePointInside(r1, r2, r3);
    if (this->isValid(pt))
      return pt;
    else
      ++attempts;
  };
  throw std::runtime_error("Object::generatePointInObject() - Unable to "
                           "generate point in object after " +
                           std::to_string(maxAttempts) + " attempts");
}

/**
* Set the geometry handler for Object
* @param[in] h is pointer to the geometry handler. don't delete this pointer in
* the calling function.
*/
void MeshObject::setGeometryHandler(boost::shared_ptr<GeometryHandler> h) {
  if (h == nullptr)
    return;
  m_handler = h;
}

/**
* Draws the Object using geometry handler, If the handler is not set then this
* function does nothing.
*/
void MeshObject::draw() const {
  if (m_handler == nullptr)
    return;
  // Render the Object
  m_handler->Render();
}

/**
* Initializes/prepares the object to be rendered, this will generate geometry
* for object,
* If the handler is not set then this function does nothing.
*/
void MeshObject::initDraw() const {
  if (m_handler == nullptr)
    return;
  // Render the Object
  m_handler->Initialize();
}

/**
* Returns the geometry handler
*/
boost::shared_ptr<GeometryHandler> MeshObject::getGeometryHandler() {
  // Check if the geometry handler is upto dated with the cache, if not then
  // cache it now.
  return m_handler;
}

/**
* Updates the geometry handler if needed
*/
void MeshObject::updateGeometryHandler() {

}

/**
* set vtkGeometryCache writer
*/
void MeshObject::setVtkGeometryCacheWriter(
  boost::shared_ptr<vtkGeometryCacheWriter> writer) {
  m_vtk_cache_writer = writer;
  updateGeometryHandler();
}

/**
* set vtkGeometryCache reader
*/
void MeshObject::setVtkGeometryCacheReader(
  boost::shared_ptr<vtkGeometryCacheReader> reader) {
  m_vtk_cache_reader = reader;
  updateGeometryHandler();
}


/**
* We hide the actual implementation of Mesh Object here
*/
int MeshObject::numberOfTriangles() const {
    return sizeof(m_triangles)/3;
}

/**
* get faces
*/
int *MeshObject::getTriangles() const {
  return nullptr;
}


/**
* get number of points
*/
int MeshObject::numberOfVertices() const {
  return sizeof(m_vertices);
}

/**
* get vertices
*/
double *MeshObject::getVertices() const {
  return nullptr;
}

/**
* get info on standard shapes
*/
void MeshObject::GetObjectGeom(int &type, std::vector<Kernel::V3D> &vectors,
                              double &myradius, double &myheight) const {
  type = 0;
  if (m_handler == nullptr)
    return;
  m_handler->GetObjectGeom(type, vectors, myradius, myheight);
}

/** Getter for the shape xml
@return the shape xml.
*/
std::string MeshObject::getShapeXML() const { return ""; }

} // NAMESPACE Geometry
} // NAMESPACE Mantid
