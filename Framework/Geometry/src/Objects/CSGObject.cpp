// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/CSGObject.h"

#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/RandomPoint.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/LineIntersectVisit.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/RegexStrings.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Tolerance.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/error_of_mean.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <memory>

#include <array>
#include <deque>
#include <random>
#include <stack>
#include <unordered_set>
#include <utility>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {

/// A shift to add/subtract to a point to test if it is an entry/exit point
constexpr double VALID_INTERCEPT_POINT_SHIFT{2.5e-05};

/**
 * Find the solid angle of a triangle defined by vectors a,b,c from point
 *"observer"
 *
 * formula (Oosterom) O=2atan([a,b,c]/(abc+(a.b)c+(a.c)b+(b.c)a))
 *
 * @param a :: first point of triangle
 * @param b :: second point of triangle
 * @param c :: third point of triangle
 * @param observer :: point from which solid angle is required
 * @return :: solid angle of triangle in Steradians.
 */
double triangleSolidAngle(const V3D &a, const V3D &b, const V3D &c, const V3D &observer) {
  const V3D ao = a - observer;
  const V3D bo = b - observer;
  const V3D co = c - observer;
  const double modao = ao.norm();
  const double modbo = bo.norm();
  const double modco = co.norm();
  const double aobo = ao.scalar_prod(bo);
  const double aoco = ao.scalar_prod(co);
  const double boco = bo.scalar_prod(co);
  const double scalTripProd = ao.scalar_prod(bo.cross_prod(co));
  const double denom = modao * modbo * modco + modco * aobo + modbo * aoco + modao * boco;
  if (denom != 0.0)
    return 2.0 * atan2(scalTripProd, denom);
  else
    return 0.0; // not certain this is correct
}

/**
 * Calculate the solid angle for a cone using triangulation.
 * @param observer :: The observer's point
 * @param centre :: The centre vector
 * @param axis :: The axis vector
 * @param radius :: The radius
 * @param height :: The height
 * @returns The solid angle value
 */
double coneSolidAngle(const V3D &observer, const Mantid::Kernel::V3D &centre, const Mantid::Kernel::V3D &axis,
                      const double radius, const double height) {
  // The cone is broken down into three pieces and then in turn broken down into
  // triangles. Any triangle that has a normal facing away from the observer
  // gives a negative solid angle and is excluded
  // For simplicity the triangulation points are constructed such that the cone
  // axis points up the +Z axis and then rotated into their final position

  const V3D axis_direction = normalize(axis);
  // Required rotation
  constexpr V3D initial_axis(0., 0., 1.0);
  const Quat transform(initial_axis, axis_direction);

  // Do the base cap which is a point at the centre and nslices points around it
  constexpr double angle_step = 2 * M_PI / Mantid::Geometry::Cone::g_NSLICES;
  // Store the (x,y) points as they are used quite frequently
  std::array<double, Mantid::Geometry::Cone::g_NSLICES> cos_table;
  std::array<double, Mantid::Geometry::Cone::g_NSLICES> sin_table;

  double solid_angle(0.0);
  for (int sl = 0; sl < Mantid::Geometry::Cone::g_NSLICES; ++sl) {
    int vertex = sl;
    cos_table[vertex] = std::cos(angle_step * vertex);
    sin_table[vertex] = std::sin(angle_step * vertex);
    V3D pt2 = V3D(radius * cos_table[vertex], radius * sin_table[vertex], 0.0);

    if (sl < Mantid::Geometry::Cone::g_NSLICES - 1) {
      vertex = sl + 1;
      cos_table[vertex] = std::cos(angle_step * vertex);
      sin_table[vertex] = std::sin(angle_step * vertex);
    } else
      vertex = 0;

    V3D pt3 = V3D(radius * cos_table[vertex], radius * sin_table[vertex], 0.0);

    transform.rotate(pt2);
    transform.rotate(pt3);
    pt2 += centre;
    pt3 += centre;

    double sa = triangleSolidAngle(centre, pt2, pt3, observer);
    if (sa > 0.0) {
      solid_angle += sa;
    }
  }

  // Now the main section
  const double z_step = height / Cone::g_NSTACKS;
  const double r_step = height / Cone::g_NSTACKS;
  double z0(0.0), z1(z_step);
  double r0(radius), r1(r0 - r_step);

  for (int st = 1; st < Cone::g_NSTACKS; ++st) {
    for (int sl = 0; sl < Cone::g_NSLICES; ++sl) {
      int vertex = sl;
      V3D pt1 = V3D(r0 * cos_table[vertex], r0 * sin_table[vertex], z0);
      if (sl < Mantid::Geometry::Cone::g_NSLICES - 1)
        vertex = sl + 1;
      else
        vertex = 0;
      V3D pt3 = V3D(r0 * cos_table[vertex], r0 * sin_table[vertex], z0);

      vertex = sl;
      V3D pt2 = V3D(r1 * cos_table[vertex], r1 * sin_table[vertex], z1);
      if (sl < Mantid::Geometry::Cone::g_NSLICES - 1)
        vertex = sl + 1;
      else
        vertex = 0;
      V3D pt4 = V3D(r1 * cos_table[vertex], r1 * sin_table[vertex], z1);
      // Rotations
      transform.rotate(pt1);
      transform.rotate(pt3);
      transform.rotate(pt2);
      transform.rotate(pt4);

      pt1 += centre;
      pt2 += centre;
      pt3 += centre;
      pt4 += centre;
      double sa = triangleSolidAngle(pt1, pt4, pt3, observer);
      if (sa > 0.0) {
        solid_angle += sa;
      }
      sa = triangleSolidAngle(pt1, pt2, pt4, observer);
      if (sa > 0.0) {
        solid_angle += sa;
      }
    }

    z0 = z1;
    r0 = r1;
    z1 += z_step;
    r1 -= r_step;
  }

  // Top section
  V3D top_centre = V3D(0.0, 0.0, height) + centre;
  transform.rotate(top_centre);
  top_centre += centre;

  for (int sl = 0; sl < Cone::g_NSLICES; ++sl) {
    int vertex = sl;
    V3D pt2 = V3D(r0 * cos_table[vertex], r0 * sin_table[vertex], height);

    if (sl < Mantid::Geometry::Cone::g_NSLICES - 1)
      vertex = sl + 1;
    else
      vertex = 0;
    V3D pt3 = V3D(r0 * cos_table[vertex], r0 * sin_table[vertex], height);

    // Rotate them to the correct axis orientation
    transform.rotate(pt2);
    transform.rotate(pt3);

    pt2 += centre;
    pt3 += centre;

    double sa = triangleSolidAngle(top_centre, pt3, pt2, observer);
    if (sa > 0.0) {
      solid_angle += sa;
    }
  }
  return solid_angle;
}

/**
 * Get the solid angle of a cuboid defined by 4 points. Simple use of triangle
 * based solid angle
 * calculation. Should work for parallel-piped as well.
 * @param observer :: point from which solid angle required
 * @param vectors :: vector of V3D - the values are the 4 points used to defined
 * the cuboid
 * @return :: solid angle of cuboid - good accuracy
 */
double cuboidSolidAngle(const V3D &observer, const std::vector<V3D> &vectors) {
  // Build bounding points, then set up map of 12 bounding
  // triangles defining the 6 surfaces of the bounding box. Using a consistent
  // ordering of points the "away facing" triangles give -ve contributions to
  // the solid angle and hence are ignored.
  std::vector<V3D> pts;
  pts.reserve(8);
  const V3D dx = vectors[1] - vectors[0];
  const V3D dz = vectors[3] - vectors[0];
  pts.emplace_back(vectors[2]);
  pts.emplace_back(vectors[2] + dx);
  pts.emplace_back(vectors[1]);
  pts.emplace_back(vectors[0]);
  pts.emplace_back(vectors[2] + dz);
  pts.emplace_back(vectors[2] + dz + dx);
  pts.emplace_back(vectors[1] + dz);
  pts.emplace_back(vectors[0] + dz);

  constexpr unsigned int ntriangles(12);
  std::vector<std::vector<int>> triMap(ntriangles, std::vector<int>(3, 0));
  triMap[0][0] = 1;
  triMap[0][1] = 4;
  triMap[0][2] = 3;
  triMap[1][0] = 3;
  triMap[1][1] = 2;
  triMap[1][2] = 1;
  triMap[2][0] = 5;
  triMap[2][1] = 6;
  triMap[2][2] = 7;
  triMap[3][0] = 7;
  triMap[3][1] = 8;
  triMap[3][2] = 5;
  triMap[4][0] = 1;
  triMap[4][1] = 2;
  triMap[4][2] = 6;
  triMap[5][0] = 6;
  triMap[5][1] = 5;
  triMap[5][2] = 1;
  triMap[6][0] = 2;
  triMap[6][1] = 3;
  triMap[6][2] = 7;
  triMap[7][0] = 7;
  triMap[7][1] = 6;
  triMap[7][2] = 2;
  triMap[8][0] = 3;
  triMap[8][1] = 4;
  triMap[8][2] = 8;
  triMap[9][0] = 8;
  triMap[9][1] = 7;
  triMap[9][2] = 3;
  triMap[10][0] = 1;
  triMap[10][1] = 5;
  triMap[10][2] = 8;
  triMap[11][0] = 8;
  triMap[11][1] = 4;
  triMap[11][2] = 1;
  double sangle = 0.0;
  for (unsigned int i = 0; i < ntriangles; i++) {
    const double sa = triangleSolidAngle(pts[triMap[i][0] - 1], pts[triMap[i][1] - 1], pts[triMap[i][2] - 1], observer);
    if (sa > 0)
      sangle += sa;
  }
  return sangle;
}

/**
 * Calculate the solid angle for a cylinder using triangulation EXCLUDING the
 * end caps.
 * @param observer :: The observer's point
 * @param centre :: The centre vector
 * @param axis :: The axis vector
 * @param radius :: The radius
 * @param height :: The height
 * @returns The solid angle value
 */
double cylinderSolidAngle(const V3D &observer, const V3D &centre, const V3D &axis, const double radius,
                          const double height, const int numberOfSlices) {
  // The cylinder is triangulated along its axis EXCLUDING the end caps so that
  // stacked cylinders give the correct value of solid angle (i.e shadowing is
  // loosely taken into account by this method) Any triangle that has a normal
  // facing away from the observer gives a negative solid angle and is excluded
  // For simplicity the triangulation points are constructed such that the cone
  // axis points up the +Z axis and then rotated into their final position

  // Required rotation
  constexpr V3D initial_axis(0., 0., 1.0);
  const Quat transform(initial_axis, axis);

  // Do the base cap which is a point at the centre and nslices points around it
  const double angle_step = 2 * M_PI / static_cast<double>(numberOfSlices);

  const double z_step = height / Cylinder::g_NSTACKS;
  double z0(0.0), z1(z_step);
  double solid_angle(0.0);
  for (int st = 1; st <= Cylinder::g_NSTACKS; ++st) {
    if (st == Cylinder::g_NSTACKS)
      z1 = height;

    for (int sl = 0; sl < numberOfSlices; ++sl) {
      double x = radius * std::cos(angle_step * sl);
      double y = radius * std::sin(angle_step * sl);
      V3D pt1 = V3D(x, y, z0);
      V3D pt2 = V3D(x, y, z1);
      int vertex = (sl + 1) % numberOfSlices;
      x = radius * std::cos(angle_step * vertex);
      y = radius * std::sin(angle_step * vertex);
      V3D pt3 = V3D(x, y, z0);
      V3D pt4 = V3D(x, y, z1);
      // Rotations
      transform.rotate(pt1);
      transform.rotate(pt3);
      transform.rotate(pt2);
      transform.rotate(pt4);

      pt1 += centre;
      pt2 += centre;
      pt3 += centre;
      pt4 += centre;

      double sa = triangleSolidAngle(pt1, pt4, pt3, observer);
      if (sa > 0.0) {
        solid_angle += sa;
      }
      sa = triangleSolidAngle(pt1, pt2, pt4, observer);
      if (sa > 0.0) {
        solid_angle += sa;
      }
    }
    z0 = z1;
    z1 += z_step;
  }

  return solid_angle;
}

/**
 * Get the solid angle of a sphere defined by centre and radius using an
 * analytic formula
 * @param observer :: point from which solid angle required
 * @param vectors :: vector of V3D - the only value is the sphere centre
 * @param radius :: sphere radius
 * @return :: solid angle of sphere
 */
double sphereSolidAngle(const V3D &observer, const std::vector<V3D> &vectors, const double radius) {
  const double distance = (observer - vectors[0]).norm();
  if (distance > radius + Tolerance) {
    const double sa = 2.0 * M_PI * (1.0 - cos(asin(radius / distance)));
    return sa;
  } else if (distance < radius - Tolerance)
    return 4.0 * M_PI; // internal point
  else
    return 2.0 * M_PI; // surface point
}
} // namespace

namespace Mantid::Geometry {

namespace {
Kernel::Logger logger("CSGObject");
}
/**
 *  Default constructor
 */
CSGObject::CSGObject() : CSGObject("") {}

/**
 *  Construct with original shape xml knowledge.
 *  @param shapeXML : string with original shape xml.
 */
CSGObject::CSGObject(std::string shapeXML)
    : m_topRule(nullptr), m_boundingBox(), AABBxMax(0), AABByMax(0), AABBzMax(0), AABBxMin(0), AABByMin(0), AABBzMin(0),
      boolBounded(false), m_objNum(0), m_handler(std::make_shared<GeometryHandler>(this)), bGeometryCaching(false),
      vtkCacheReader(std::shared_ptr<vtkGeometryCacheReader>()),
      vtkCacheWriter(std::shared_ptr<vtkGeometryCacheWriter>()), m_shapeXML(std::move(shapeXML)), m_id(),
      m_material(std::make_unique<Material>()) {}

/**
 * Copy constructor
 * @param A :: The object to initialise this copy from
 */
CSGObject::CSGObject(const CSGObject &A) : CSGObject() { *this = A; }

/**
 * Assignment operator
 * @param A :: Object to copy
 * @return *this
 */
CSGObject &CSGObject::operator=(const CSGObject &A) {
  if (this != &A) {
    m_topRule = (A.m_topRule) ? A.m_topRule->clone() : nullptr;
    AABBxMax = A.AABBxMax;
    AABByMax = A.AABByMax;
    AABBzMax = A.AABBzMax;
    AABBxMin = A.AABBxMin;
    AABByMin = A.AABByMin;
    AABBzMin = A.AABBzMin;
    boolBounded = A.boolBounded;
    m_objNum = A.m_objNum;
    m_handler = A.m_handler->clone();
    bGeometryCaching = A.bGeometryCaching;
    vtkCacheReader = A.vtkCacheReader;
    vtkCacheWriter = A.vtkCacheWriter;
    m_shapeXML = A.m_shapeXML;
    m_id = A.m_id;
    m_material = std::make_unique<Material>(A.material());

    if (m_topRule)
      createSurfaceList();
  }
  return *this;
}

/// Destructor in .cpp so we can forward declare Rule class.
CSGObject::~CSGObject() = default;

/**
 * @param material The new Material that the object is composed from
 */
void CSGObject::setMaterial(const Kernel::Material &material) { m_material = std::make_unique<Material>(material); }

/**
 * @return The Material that the object is composed from
 */
const Kernel::Material &CSGObject::material() const { return *m_material; }

/**
 * Returns whether this object has a valid shape
 * @returns True if the surface list is populated and there is a
 * defined m_topRule, false otherwise.
 */
bool CSGObject::hasValidShape() const {
  // Assume invalid shape if object has no 'm_topRule' or surfaces
  return (m_topRule != nullptr && !m_surList.empty());
}

/**
 * Object line ==  cell
 * @param objName :: Object name
 * @param lineStr :: Input string must be :  {rules}
 * @returns 1 on success and zero on failure
 */
int CSGObject::setObject(const int objName, const std::string &lineStr) {
  // Split line
  // Does the string now contain junk...
  static const boost::regex letters("[a-zA-Z]");
  if (Mantid::Kernel::Strings::StrLook(lineStr, letters))
    return 0;

  if (procString(lineStr)) // this currently does not fail:
  {
    m_surList.clear();
    m_objNum = objName;
    return 1;
  }

  // failure
  return 0;
}

/**
 * Returns just the cell string object
 * @param MList :: List of indexable Hulls
 * @todo Break infinite recursion
 */
void CSGObject::convertComplement(const std::map<int, CSGObject> &MList)

{
  this->procString(this->cellStr(MList));
}

/**
 * Returns just the cell string object
 * @param MList :: List of indexable Hulls
 * @return Cell String (from m_topRule)
 * @todo Break infinite recursion
 */
std::string CSGObject::cellStr(const std::map<int, CSGObject> &MList) const {
  std::string TopStr = this->topRule()->display();
  std::string::size_type pos = TopStr.find('#');
  std::ostringstream cx;
  while (pos != std::string::npos) {
    pos++;
    cx << TopStr.substr(0, pos); // Everything including the #
    int cN(0);
    const int nLen = Mantid::Kernel::Strings::convPartNum(TopStr.substr(pos), cN);
    if (nLen > 0) {
      cx << "(";
      auto vc = MList.find(cN);
      if (vc == MList.end())
        throw Kernel::Exception::NotFoundError("Not found in the list of indexable hulls (Object::cellStr)", cN);
      // Not the recursion :: This will cause no end of problems
      // if there is an infinite loop.
      cx << vc->second.cellStr(MList);
      cx << ") ";
      pos += nLen;
    }
    TopStr.erase(0, pos);
    pos = TopStr.find('#');
  }
  cx << TopStr;
  return cx.str();
}

/*
 * Calculate if there are any complementary components in
 * the object. That is lines with #(....)
 * @throw ColErr::ExBase :: Error with processing
 * @param lineStr :: Input string must:  ID Mat {Density}  {rules}
 * @param cellNum :: Number for cell since we don't have one
 * @retval 0 on no work to do
 * @retval 1 :: A (maybe there are many) #(...) object found
 */
int CSGObject::complementaryObject(const int cellNum, std::string &lineStr) {
  std::string::size_type posA = lineStr.find("#(");
  // No work to do ?
  if (posA == std::string::npos)
    return 0;
  posA += 2;

  // First get the area to be removed
  int numBrackets;
  std::string::size_type posB;
  posB = lineStr.find_first_of("()", posA);
  if (posB == std::string::npos)
    throw std::runtime_error("Object::complement :: " + lineStr);

  numBrackets = (lineStr[posB] == '(') ? 1 : 0;
  while (posB != std::string::npos && numBrackets) {
    posB = lineStr.find_first_of("()", posB);
    if (posB == std::string::npos)
      break;
    numBrackets += (lineStr[posB] == '(') ? 1 : -1;
    posB++;
  }

  std::string Part = lineStr.substr(posA, posB - (posA + 1));

  m_objNum = cellNum;
  // cppcheck-suppress knownConditionTrueFalse
  if (procString(Part)) {
    // this validates if Part is a valid procString, a variable specific to this file
    m_surList.clear();
    lineStr.erase(posA - 1, posB + 1); // Delete brackets ( Part ) .
    std::ostringstream CompCell;
    CompCell << cellNum << " ";
    lineStr.insert(posA - 1, CompCell.str());
    return 1;
  }

  throw std::runtime_error("Object::complement :: " + Part);
}

/**
 * Determine if the object has a complementary object
 * @retval 1 :: true
 * @retval 0 :: false
 */
int CSGObject::hasComplement() const {

  if (m_topRule)
    return m_topRule->isComplementary();
  return 0;
}

/**
 * Goes through the cell objects and adds the pointers
 * to the SurfPoint keys (using their keyN)
 * @param surfMap :: Map of surface Keys and Surface Pointers
 * @retval 1000+ keyNumber :: Error with keyNumber
 * @retval 0 :: successfully populated all the whole Object.
 */
int CSGObject::populate(const std::map<int, std::shared_ptr<Surface>> &surfMap) {
  std::deque<Rule *> rules;
  rules.emplace_back(m_topRule.get());
  while (!rules.empty()) {
    Rule *T1 = rules.front();
    rules.pop_front();
    if (T1) {
      // if an actual surface process :
      auto *surface = dynamic_cast<SurfPoint *>(T1);
      if (surface) {
        // Ensure that we have a it in the surface list:
        auto mapFound = surfMap.find(surface->getKeyN());
        if (mapFound != surfMap.end()) {
          surface->setKey(mapFound->second);
        } else {
          throw Kernel::Exception::NotFoundError("Object::populate", surface->getKeyN());
        }
      }
      // Not a surface : Determine leaves etc and add to stack:
      else {
        Rule *TA = T1->leaf(0);
        Rule *TB = T1->leaf(1);
        if (TA)
          rules.emplace_back(TA);
        if (TB)
          rules.emplace_back(TB);
      }
    }
  }
  createSurfaceList();
  return 0;
}

/**
 * This takes a string lineStr, finds the first two
 * Rxxx function, determines their join type
 * make the rule,  adds to vector then removes two old rules from
 * the vector, updates string
 * @param lineStr :: String to process
 * @param ruleMap :: Map of rules (added to)
 * @param compUnit :: Last computed unit
 * @retval 0 :: No rule to find
 * @retval 1 :: A rule has been combined
 */
int CSGObject::procPair(std::string &lineStr, std::map<int, std::unique_ptr<Rule>> &ruleMap, int &compUnit) const

{
  unsigned int Rstart;
  unsigned int Rend;
  int Ra, Rb;

  for (Rstart = 0; Rstart < lineStr.size() && lineStr[Rstart] != 'R'; Rstart++)
    ;

  int type = 0; // intersection

  // plus 1 to skip 'R'
  if (Rstart == lineStr.size() || !Mantid::Kernel::Strings::convert(lineStr.c_str() + Rstart + 1, Ra) ||
      ruleMap.find(Ra) == ruleMap.end())
    return 0;

  for (Rend = Rstart + 1; Rend < lineStr.size() && lineStr[Rend] != 'R'; Rend++) {
    if (lineStr[Rend] == ':')
      type = 1; // make union
  }
  if (Rend == lineStr.size() || !Mantid::Kernel::Strings::convert(lineStr.c_str() + Rend + 1, Rb) ||
      ruleMap.find(Rb) == ruleMap.end()) {
    // No second rule but we did find the first one
    compUnit = Ra;
    return 0;
  }
  // Get end of number (digital)
  for (Rend++; Rend < lineStr.size() && lineStr[Rend] >= '0' && lineStr[Rend] <= '9'; Rend++)
    ;

  // Get rules
  auto Join =
      (type) ? std::unique_ptr<Rule>(std::make_unique<Union>(std::move(ruleMap[Ra]), std::move(ruleMap[Rb])))
             : std::unique_ptr<Rule>(std::make_unique<Intersection>(std::move(ruleMap[Ra]), std::move(ruleMap[Rb])));
  ruleMap[Ra] = std::move(Join);
  ruleMap.erase(ruleMap.find(Rb));

  // Remove space round pair
  int strPos;
  for (strPos = Rstart - 1; strPos >= 0 && lineStr[strPos] == ' '; strPos--)
    ;
  Rstart = (strPos < 0) ? 0 : strPos;
  for (strPos = Rend; strPos < static_cast<int>(lineStr.size()) && lineStr[strPos] == ' '; strPos++)
    ;
  Rend = strPos;

  std::stringstream newRuleStr;
  newRuleStr << " R" << Ra << " ";
  lineStr.replace(Rstart, Rend, newRuleStr.str());
  compUnit = Ra;
  return 1;
}

/**
 * Takes a Rule item and makes it a complementary group
 * @param ruleItem :: to encapsulate
 * @returns the complementary group
 */
std::unique_ptr<CompGrp> CSGObject::procComp(std::unique_ptr<Rule> ruleItem) const {
  if (!ruleItem)
    return std::make_unique<CompGrp>();

  Rule *Pptr = ruleItem->getParent();
  const Rule *RItemptr = ruleItem.get();
  auto CG = std::make_unique<CompGrp>(Pptr, std::move(ruleItem));
  if (Pptr) {
    const int Ln = Pptr->findLeaf(RItemptr);
    Pptr->setLeaf(std::move(CG), Ln);
    // CG already in tree. Return empty object.
    return std::make_unique<CompGrp>();
  }
  return CG;
}

/**
* - (a) Uses the Surface list to check those surface
* that the point is on.
* - (b) Creates a list of normals to the touching surfaces
* - (c) Checks if normals and "normal pair bisection vector" are contary.
* If any are found to be so the point is
* on a surface.
* - (d) Return 1 / 0 depending on test (c)

* \todo This needs to be completed to deal with apex points
* In the case of a apex (e.g. top of a pyramid) you need
* to iterate over all clusters of points on the Snorm
* ie. sum of 2, sum of 3 sum of 4. etc. to be certain
* to get a correct normal test.

* @param point :: Point to check
* @returns true if the point is on the surface
*/
bool CSGObject::isOnSide(const Kernel::V3D &point) const {
  std::vector<Kernel::V3D> Snorms; // Normals from the contact surface.
  Snorms.reserve(m_surList.size());

  for (auto vc = m_surList.begin(); vc != m_surList.end(); ++vc) {
    if ((*vc)->onSurface(point)) {
      Snorms.emplace_back((*vc)->surfaceNormal(point));
      // can check direct normal here since one success
      // means that we can return true and finish
      if (!checkSurfaceValid(point, Snorms.back()))
        return true;
    }
  }
  Kernel::V3D NormPair;
  for (auto xs = Snorms.begin(); xs != Snorms.end(); ++xs)
    for (auto ys = std::next(xs); ys != Snorms.end(); ++ys) {
      NormPair = (*ys) + (*xs);
      try {
        NormPair.normalize();
        if (!checkSurfaceValid(point, NormPair))
          return true;
      } catch (std::runtime_error &) {
      }
    }
  // Ok everthing failed
  return false;
}

/**
 * Determine if a point is valid by checking both
 * directions of the normal away from the line
 * A good point will have one valid and one invalid.
 * @param point :: Point on a basic surface to check
 * @param direction :: Direction +/- to be checked
 * @retval +1 ::  Point outlayer (ie not in object)
 * @retval -1 :: Point included (e.g at convex intersection)
 * @retval 0 :: success
 */
int CSGObject::checkSurfaceValid(const Kernel::V3D &point, const Kernel::V3D &direction) const {
  int status(0);
  Kernel::V3D tmp = point + direction * (Kernel::Tolerance * 5.0);
  status = (!isValid(tmp)) ? 1 : -1;
  tmp -= direction * (Kernel::Tolerance * 10.0);
  status += (!isValid(tmp)) ? 1 : -1;
  return status / 2;
}

/**
 * Determines is point is within the object or on the surface
 * @param point :: Point to be tested
 * @returns 1 if true and 0 if false
 */
bool CSGObject::isValid(const Kernel::V3D &point) const {
  if (!m_topRule)
    return false;
  return m_topRule->isValid(point);
}

/**
 * Determines is group of surface maps are valid
 * @param SMap :: map of SurfaceNumber : status
 * @returns 1 if true and 0 if false
 */
bool CSGObject::isValid(const std::map<int, int> &SMap) const {
  if (!m_topRule)
    return false;
  return m_topRule->isValid(SMap);
}

/**
 * Uses the topRule* to create a surface list
 * by iterating throught the tree
 * @param outFlag :: Sends output to standard error if true
 * @return 1 (should be number of surfaces)
 */
int CSGObject::createSurfaceList(const int outFlag) {
  m_surList.clear();
  std::stack<const Rule *> TreeLine;
  TreeLine.push(m_topRule.get());
  while (!TreeLine.empty()) {
    const Rule *tmpA = TreeLine.top();
    TreeLine.pop();
    const Rule *tmpB = tmpA->leaf(0);
    const Rule *tmpC = tmpA->leaf(1);
    if (tmpB || tmpC) {
      if (tmpB)
        TreeLine.push(tmpB);
      if (tmpC)
        TreeLine.push(tmpC);
    } else {
      const auto *SurX = dynamic_cast<const SurfPoint *>(tmpA);
      if (SurX) {
        m_surList.emplace_back(SurX->getKey());
      }
    }
  }
  // Remove duplicates without reordering
  std::unordered_set<const Surface *> uniqueSurfacePtrs;

  auto newEnd = std::remove_if(m_surList.begin(), m_surList.end(), [&uniqueSurfacePtrs](const Surface *sPtr) {
    if (uniqueSurfacePtrs.find(sPtr) != std::end(uniqueSurfacePtrs)) {
      return true;
    } else {
      uniqueSurfacePtrs.insert(sPtr);
      return false;
    };
  });
  m_surList.erase(newEnd, m_surList.end());

  if (outFlag) {

    std::vector<const Surface *>::const_iterator vc;
    for (vc = m_surList.begin(); vc != m_surList.end(); ++vc) {
      logger.debug() << "Point == " << *vc << '\n';
      logger.debug() << (*vc)->getName() << '\n';
    }
  }
  return 1;
}

/**
 * Returns all of the numbers of surfaces
 * @return Surface numbers
 */
std::vector<int> CSGObject::getSurfaceIndex() const {
  std::vector<int> out;
  transform(m_surList.begin(), m_surList.end(), std::insert_iterator<std::vector<int>>(out, out.begin()),
            std::mem_fn(&Surface::getName));
  return out;
}

/**
 * Removes a surface and then re-builds the
 * cell. This could be done by just removing
 * the surface from the object.
 * @param surfNum :: Number for the surface
 * @return number of surfaces removed
 */
int CSGObject::removeSurface(const int surfNum) {
  if (!m_topRule)
    return -1;
  const int nRemoved = Rule::removeItem(m_topRule, surfNum);
  if (nRemoved)
    createSurfaceList();
  return nRemoved;
}

/**
 * Removes a surface and then re-builds the cell.
 * @param surfNum :: Number for the surface
 * @param newSurfNum :: New surface number
 * @param surfPtr :: Surface pointer for surface NsurfN
 * @return number of surfaces substituted
 */
int CSGObject::substituteSurf(const int surfNum, const int newSurfNum, const std::shared_ptr<Surface> &surfPtr) {
  if (!m_topRule)
    return 0;
  const int out = m_topRule->substituteSurf(surfNum, newSurfNum, surfPtr);
  if (out)
    createSurfaceList();
  return out;
}

/**
 * Prints almost everything
 */
void CSGObject::print() const {
  std::deque<Rule *> rst;
  std::vector<int> Cells;
  int Rcount(0);
  rst.emplace_back(m_topRule.get());
  Rule *TA, *TB; // Temp. for storage

  while (!rst.empty()) {
    const Rule *T1 = rst.front();
    rst.pop_front();
    if (T1) {
      Rcount++;
      const auto *surface = dynamic_cast<const SurfPoint *>(T1);
      if (surface)
        Cells.emplace_back(surface->getKeyN());
      else {
        TA = T1->leaf(0);
        TB = T1->leaf(1);
        if (TA)
          rst.emplace_back(TA);
        if (TB)
          rst.emplace_back(TB);
      }
    }
  }

  logger.debug() << "Name == " << m_objNum << '\n';
  logger.debug() << "Rules == " << Rcount << '\n';
  std::vector<int>::const_iterator mc;
  logger.debug() << "Surface included == ";
  for (mc = Cells.begin(); mc < Cells.end(); ++mc) {
    logger.debug() << (*mc) << " ";
  }
  logger.debug() << '\n';
}

/**
 * Takes the complement of a group
 */
void CSGObject::makeComplement() {
  std::unique_ptr<Rule> NCG = procComp(std::move(m_topRule));
  m_topRule = std::move(NCG);
}

/**
 * Displays the rule tree
 */
void CSGObject::printTree() const {
  logger.debug() << "Name == " << m_objNum << '\n';
  logger.debug() << m_topRule->display() << '\n';
}

/**
 * Write the object to a string.
 * This includes only rules.
 * @return Object Line
 */
std::string CSGObject::cellCompStr() const {
  std::ostringstream objStr;
  if (m_topRule)
    objStr << m_topRule->display();
  return objStr.str();
}

/**
 * Write the object to a string.
 * This includes the Name but not post-fix operators
 * @return Object Line
 */
std::string CSGObject::str() const {
  std::ostringstream objStr;
  if (m_topRule) {
    objStr << m_objNum << " ";
    objStr << m_topRule->display();
  }
  return objStr.str();
}

/**
 * Write the object to a standard stream
 * in standard MCNPX output format.
 * @param outStream :: Output stream (required for multiple std::endl)
 */
void CSGObject::write(std::ostream &outStream) const {
  std::ostringstream objStr;
  objStr.precision(10);
  objStr << str();
  Mantid::Kernel::Strings::writeMCNPX(objStr.str(), outStream);
}

/**
 * Processes the cell string. This is an internal function
 * to process a string with - String type has #( and ( )
 * @param lineStr :: String value
 * @returns 1 on success
 */
int CSGObject::procString(const std::string &lineStr) {
  m_topRule = nullptr;
  std::map<int, std::unique_ptr<Rule>> RuleList; // List for the rules
  int Ridx = 0;                                  // Current index (not necessary size of RuleList
  // SURFACE REPLACEMENT
  // Now replace all free planes/Surfaces with appropiate Rxxx
  std::unique_ptr<SurfPoint> TmpR; // Tempory Rule storage position
  std::unique_ptr<CompObj> TmpO;   // Tempory Rule storage position

  std::string Ln = lineStr;
  // Remove all surfaces :
  std::ostringstream cx;
  const std::string::size_type length = Ln.length();
  for (size_t i = 0; i < length; i++) {
    if (isdigit(Ln[i]) || Ln[i] == '-') {
      int SN;
      int nLen = Mantid::Kernel::Strings::convPartNum(Ln.substr(i), SN);
      if (!nLen)
        throw std::invalid_argument("Invalid surface string in Object::ProcString : " + lineStr);
      // Process #Number
      if (i != 0 && Ln[i - 1] == '#') {
        TmpO = std::make_unique<CompObj>();
        TmpO->setObjN(SN);
        RuleList[Ridx] = std::move(TmpO);
      } else // Normal rule
      {
        TmpR = std::make_unique<SurfPoint>();
        TmpR->setKeyN(SN);
        RuleList[Ridx] = std::move(TmpR);
      }
      cx << " R" << Ridx << " ";
      Ridx++;
      i += nLen;
    }
    if (i < length)
      cx << Ln[i];
  }
  Ln = cx.str();
  // PROCESS BRACKETS

  int brack_exists = 1;
  while (brack_exists) {
    std::string::size_type rbrack = Ln.find(')');
    std::string::size_type lbrack = Ln.rfind('(', rbrack);
    if (rbrack != std::string::npos && lbrack != std::string::npos) {
      std::string Lx = Ln.substr(lbrack + 1, rbrack - lbrack - 1);
      // Check to see if a #( unit
      int compUnit(0);
      while (procPair(Lx, RuleList, compUnit))
        ;
      Ln.replace(lbrack, 1 + rbrack - lbrack, Lx);
      // Search back and find if # ( exists.
      int hCnt;
      for (hCnt = static_cast<int>(lbrack) - 1; hCnt >= 0 && isspace(Ln[hCnt]); hCnt--)
        ;
      if (hCnt >= 0 && Ln[hCnt] == '#') {
        RuleList[compUnit] = procComp(std::move(RuleList[compUnit]));
        Ln.erase(hCnt, lbrack - hCnt);
      }
    } else
      brack_exists = 0;
  }
  // Do outside loop...
  int nullInt;
  while (procPair(Ln, RuleList, nullInt)) {
  }

  if (RuleList.size() == 1) {
    m_topRule = std::move((RuleList.begin())->second);
  } else {
    throw std::logic_error("Object::procString() - Unexpected number of "
                           "surface rules found. Expected=1, found=" +
                           std::to_string(RuleList.size()));
  }
  return 1;
}

/**
 * Given a track, fill the track with valid section
 * @param track :: Initial track
 * @return Number of segments added
 */
int CSGObject::interceptSurface(Geometry::Track &track) const {
  // Number of intersections original track
  int originalCount = track.count();

  // Loop over all the surfaces to get the intercepts, i.e. populating
  // points into LI
  LineIntersectVisit LI(track.startPoint(), track.direction());

  for (auto &surface : m_surList) {
    surface->acceptVisitor(LI);
  }

  // Call the pruner so that we don't have to worry about the duplicates and
  // the order
  LI.sortAndRemoveDuplicates();

  // IPoints: std::vector<Geometry::V3D>
  const auto &IPoints(LI.getPoints());
  // dPoints: std::vector<double>, distance to the start point for each point
  const auto &dPoints(LI.getDistance());
  // nPoints: size_t, total number of points, for most shape, this number should
  //          be a single digit number
  const size_t nPoints(IPoints.size());

  // Loop over all the points and add them to the track
  for (size_t i = 0; i < nPoints; i++) {
    // skip over the points that are before the starting points
    if (dPoints[i] < 0)
      continue;

    //
    const auto &currentPt(IPoints[i]);
    const auto &prePt((i == 0 || dPoints[i - 1] <= 0) ? track.startPoint() : IPoints[i - 1]);
    const auto &nextPt(i + 1 < nPoints ? IPoints[i + 1] : currentPt + currentPt - prePt);

    // get the intercept type
    const TrackDirection trackType = calcValidTypeBy3Points(prePt, currentPt, nextPt);
    // only record the intercepts that is interacting with the shape directly
    if (trackType != TrackDirection::INVALID) {
      track.addPoint(trackType, currentPt, *this);
    }
  }

  track.buildLink();
  // Return number of track segments added
  return (track.count() - originalCount);
}

/**
 * Compute the distance to the first point of intersection with the surface
 * @param track Track defining start/direction
 * @return The distance to the object
 * @throws std::runtime_error if no intersection was found
 */
double CSGObject::distance(const Geometry::Track &track) const {
  LineIntersectVisit LI(track.startPoint(), track.direction());
  for (auto &surface : m_surList) {
    surface->acceptVisitor(LI);
  }
  LI.sortAndRemoveDuplicates();
  const auto &distances(LI.getDistance());
  if (!distances.empty()) {
    return std::abs(*std::min_element(std::begin(distances), std::end(distances)));
  } else {
    std::ostringstream os;
    os << "Unable to find intersection with object with track starting at " << track.startPoint() << " in direction "
       << track.direction() << "\n";
    throw std::runtime_error(os.str());
  }
}

/**
 * Calculate if a point PT is a valid point on the track
 * @param point :: Point to calculate from.
 * @param uVec :: Unit vector of the track
 * @retval 0 :: Not valid / double valid
 * @retval 1 :: Entry point
 * @retval -1 :: Exit Point
 */
TrackDirection CSGObject::calcValidType(const Kernel::V3D &point, const Kernel::V3D &uVec) const {
  // NOTE: This method is sensitive to the geometry dimension, and will lead to
  //       incorrect identification due to the value of VALID_INTERCEPT_POINT_SHIFT
  //       being either too large or too small.
  const Kernel::V3D shift(uVec * VALID_INTERCEPT_POINT_SHIFT);
  const int flagA = isValid(point - shift);
  const int flagB = isValid(point + shift);
  if (!(flagA ^ flagB))
    return Geometry::TrackDirection::INVALID;
  return (flagA) ? Geometry::TrackDirection::LEAVING : Geometry::TrackDirection::ENTERING;
}

/**
 * Check if an intercept is guiding the ray into the shape or leaving the shape
 * @param prePt :: the previous point on the Line
 * @param curPt :: the current point on the Line
 * @param nxtPt :: Unit vector of the track
 * @retval 1 :: Entry point
 * @retval -1 :: Exit Point
 * @retval 0 :: Not valid / double valid
 */
TrackDirection CSGObject::calcValidTypeBy3Points(const Kernel::V3D &prePt, const Kernel::V3D &curPt,
                                                 const Kernel::V3D &nxtPt) const {
  // upstream point
  const auto upstreamPt = (prePt + curPt) * 0.5;
  const auto upstreamPtInsideShape = isValid(upstreamPt);
  // downstream point
  const auto downstreamPt = (curPt + nxtPt) * 0.5;
  const auto downstreamPtInsideShape = isValid(downstreamPt);
  // NOTE:
  // When the track is parallel to the shape, it can still intersect with its
  // component (infinite) surface.
  // __Legends__
  //  o-->: track
  //  o:    track starting point
  //  m:    upstreamPt
  //  x:    currentPt
  //  d:    downstreamPt
  //              |                    |               o
  //              |                    |               |
  //     ---------|--------------------|---------------x------
  //              |       SHAPE        |               |
  //       o--m---x-d---->     o---m---x-d---->        v Invalid
  //              |   Entering         |    Leaving
  //     ---------|--------------------|----------------------
  //              |                    |
  if (!(upstreamPtInsideShape ^ downstreamPtInsideShape))
    return Geometry::TrackDirection::INVALID;
  else
    return (upstreamPtInsideShape) ? Geometry::TrackDirection::LEAVING : Geometry::TrackDirection::ENTERING;
}

/**
 * Find solid angle of object wrt the observer.
 * This interface routine calls either getTriangleSolidAngle or
 * getRayTraceSolidAngle.
 * Choice made on number of triangles in the discrete surface representation.
 * @param params :: point to measure solid angle from, and number of cylinder slices
 * @return :: estimate of solid angle of object. Accuracy depends on object
 * shape.
 */
double CSGObject::solidAngle(const SolidAngleParams &params) const {
  if (this->numberOfTriangles() > 30000)
    return rayTraceSolidAngle(params.observer());
  return triangulatedSolidAngle(params);
}

/**
 * Find solid angle of object wrt the observer with a scaleFactor for the
 * object.
 * @param params :: point to measure solid angle from, and number of cylinder slices
 * @param scaleFactor :: V3D giving scaling of the object
 * @return :: estimate of solid angle of object. Accuracy depends on
 * triangulation quality.
 */
double CSGObject::solidAngle(const SolidAngleParams &params, const Kernel::V3D &scaleFactor) const {
  return triangulatedSolidAngle(params, scaleFactor);
}

/**
 * Given an observer position find the approximate solid angle of the object
 * @param observer :: position of the observer (V3D)
 * @return Solid angle in steradians (+/- 1% if accurate bounding box available)
 */
double CSGObject::rayTraceSolidAngle(const Kernel::V3D &observer) const {
  // Calculation of solid angle as numerical double integral over all angles.
  // This could be optimized further e.g. by using a light weight version of
  // the interceptSurface method - this does more work than is necessary in this
  // application.
  // Accuracy is of the order of 1% for objects with an accurate bounding box,
  // though less in the case of high aspect ratios.
  //
  // resBB controls accuracy and cost - linear accuracy improvement with
  // increasing res, but quadratic increase in run time. If no bounding box
  // found, resNoBB used instead.
  const int resNoBB = 200, resPhiMin = 10;
  int res = resNoBB, itheta, jphi, resPhi;
  double theta, phi, sum, dphi, dtheta;
  if (this->isValid(observer) && !this->isOnSide(observer))
    return 4 * M_PI; // internal point
  if (this->isOnSide(observer))
    return 2 * M_PI; // this is wrong if on an edge
  // Use BB if available, and if observer not within it
  const BoundingBox &boundingBox = getBoundingBox();
  double thetaMax = M_PI;
  bool useBB = false, usePt = false;
  Kernel::V3D ptInObject, axis;
  Quat zToPt;

  // Is the bounding box a reasonable one?
  if (boundingBox.isNonNull() && !boundingBox.isPointInside(observer)) {
    useBB = usePt = true;
    thetaMax = boundingBox.angularWidth(observer);
    ptInObject = boundingBox.centrePoint();
    const int resBB = 100;
    res = resBB;
  }
  // Try and find a point in the object if useful bounding box not found
  if (!useBB) {
    usePt = getPointInObject(ptInObject) == 1;
  }
  if (usePt) {
    // found point in object, now get rotation that maps z axis to this
    // direction from observer
    ptInObject -= observer;
    double theta0 = -180.0 / M_PI * acos(ptInObject.Z() / ptInObject.norm());
    Kernel::V3D zDir(0.0, 0.0, 1.0);
    axis = ptInObject.cross_prod(zDir);
    if (axis.nullVector())
      axis = Kernel::V3D(1.0, 0.0, 0.0);
    zToPt(theta0, axis);
  }
  dtheta = thetaMax / res;
  int count = 0, countPhi;
  sum = 0.;
  for (itheta = 1; itheta <= res; itheta++) {
    // itegrate theta from 0 to maximum from bounding box, or PI otherwise
    theta = thetaMax * (itheta - 0.5) / res;
    resPhi = static_cast<int>(res * sin(theta));
    if (resPhi < resPhiMin)
      resPhi = resPhiMin;
    dphi = 2 * M_PI / resPhi;
    countPhi = 0;
    for (jphi = 1; jphi <= resPhi; jphi++) {
      // integrate phi from 0 to 2*PI
      phi = 2.0 * M_PI * (jphi - 0.5) / resPhi;
      Kernel::V3D dir(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      if (usePt)
        zToPt.rotate(dir);
      if (!useBB || boundingBox.doesLineIntersect(observer, dir)) {
        Track tr(observer, dir);
        if (this->interceptSurface(tr) > 0) {
          sum += dtheta * dphi * sin(theta);
          countPhi++;
        }
      }
    }
    // this break (only used if no BB defined) may be wrong if object has hole
    // in middle
    if (!useBB && countPhi == 0)
      break;
    count += countPhi;
  }
  if (!useBB && count < resPhiMin + 1) {
    // case of no bound box defined and object has few if any points in sum
    // redo integration on finer scale
    thetaMax = thetaMax * (itheta - 0.5) / res;
    dtheta = thetaMax / res;
    sum = 0;
    for (itheta = 1; itheta <= res; itheta++) {
      theta = thetaMax * (itheta - 0.5) / res;
      resPhi = static_cast<int>(res * sin(theta));
      if (resPhi < resPhiMin)
        resPhi = resPhiMin;
      dphi = 2 * M_PI / resPhi;
      countPhi = 0;
      for (jphi = 1; jphi <= resPhi; jphi++) {
        phi = 2.0 * M_PI * (jphi - 0.5) / resPhi;
        Kernel::V3D dir(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
        if (usePt)
          zToPt.rotate(dir);
        Track tr(observer, dir);
        if (this->interceptSurface(tr) > 0) {
          sum += dtheta * dphi * sin(theta);
          countPhi++;
        }
      }
      if (countPhi == 0)
        break;
    }
  }

  return sum;
}

/**
 * Find solid angle of object from point "observer" using the
 * OC triangulation of the object, if it exists
 *
 * @param params :: Point from which solid angle is required, and number of cylinder slices
 * @return the solid angle
 */
double CSGObject::triangulatedSolidAngle(const SolidAngleParams &params) const {
  //
  // Because the triangles from OC are not consistently ordered wrt their
  // outward normal internal points give incorrect solid angle. Surface
  // points are difficult to get right with the triangle based method.
  // Hence catch these two (unlikely) cases.
  const auto &observer = params.observer();
  const BoundingBox &boundingBox = this->getBoundingBox();
  if (boundingBox.isNonNull() && boundingBox.isPointInside(observer)) {
    if (isValid(observer)) {
      if (isOnSide(observer))
        return (2.0 * M_PI);
      else
        return (4.0 * M_PI);
    }
  }

  // If the object is a simple shape use the special methods
  double height(0.0), radius(0.0), innerRadius(0.0);
  detail::ShapeInfo::GeometryShape type;
  std::vector<Mantid::Kernel::V3D> geometry_vectors;
  // Maximum of 4 vectors depending on the type
  geometry_vectors.reserve(4);
  this->GetObjectGeom(type, geometry_vectors, innerRadius, radius, height);
  auto nTri = this->numberOfTriangles();

  // Cylinders are by far the most frequently used
  switch (type) {
  case detail::ShapeInfo::GeometryShape::CUBOID:
    return cuboidSolidAngle(observer, geometry_vectors);
    break;
  case detail::ShapeInfo::GeometryShape::SPHERE:
    return sphereSolidAngle(observer, geometry_vectors, radius);
    break;
  case detail::ShapeInfo::GeometryShape::CYLINDER:
    return cylinderSolidAngle(observer, geometry_vectors[0], geometry_vectors[1], radius, height,
                              params.cylinderSlices());
    break;
  case detail::ShapeInfo::GeometryShape::CONE:
    return coneSolidAngle(observer, geometry_vectors[0], geometry_vectors[1], radius, height);
    break;
  default:
    if (nTri == 0) // Fall back to raytracing if there are no triangles
    {
      return rayTraceSolidAngle(observer);
    } else { // Compute a generic shape that has been triangulated
      const auto &vertices = this->getTriangleVertices();
      const auto &faces = this->getTriangleFaces();
      double sangle(0.0), sneg(0.0);
      for (size_t i = 0; i < nTri; i++) {
        int p1 = faces[i * 3], p2 = faces[i * 3 + 1], p3 = faces[i * 3 + 2];
        V3D vp1 = V3D(vertices[3 * p1], vertices[3 * p1 + 1], vertices[3 * p1 + 2]);
        V3D vp2 = V3D(vertices[3 * p2], vertices[3 * p2 + 1], vertices[3 * p2 + 2]);
        V3D vp3 = V3D(vertices[3 * p3], vertices[3 * p3 + 1], vertices[3 * p3 + 2]);
        double sa = triangleSolidAngle(vp1, vp2, vp3, observer);
        if (sa > 0.0) {
          sangle += sa;
        } else {
          sneg += sa;
        }
      }
      /* We assume that objects are opaque to neutrons and that objects define
       * closed surfaces which are convex. For such objects negative solid angle
       * equals positive solid angle. This is true providing that the winding
       * order is defined properly such that the contribution from each triangle
       * w.r.t the observer gets counted to either the negative or positive
       * contribution correctly. If that is done correctly then it would only be
       * necessary to consider the positive contribution to the solid angle.
       *
       * The following provides a fix to situations where the winding order is
       * incorrectly defined. It does not matter if the contribution is positive
       * or negative since we take the average.
       */
      return 0.5 * (sangle - sneg);
    }
  }
}
/**
 * Find solid angle of object from point "observer" using the
 * OC triangulation of the object, if it exists. This method expects a
 * scaling vector scaleFactor that scales the three axes.
 *
 * @param params :: Point from which solid angle is required, and number of cylinder slices
 * @param scaleFactor :: V3D each component giving the scaling of the object
 *only (not observer)
 * @return the solid angle
 */
double CSGObject::triangulatedSolidAngle(const SolidAngleParams &params, const V3D &scaleFactor) const {
  //
  // Because the triangles from OC are not consistently ordered wrt their
  // outward normal internal points give incorrect solid angle. Surface
  // points are difficult to get right with the triangle based method.
  // Hence catch these two (unlikely) cases.
  const auto &observer = params.observer();
  const BoundingBox &boundingBox = this->getBoundingBox();
  double sx = scaleFactor[0], sy = scaleFactor[1], sz = scaleFactor[2];
  const V3D sObserver = observer;
  if (boundingBox.isNonNull() && boundingBox.isPointInside(sObserver)) {
    if (isValid(sObserver)) {
      if (isOnSide(sObserver))
        return (2.0 * M_PI);
      else
        return (4.0 * M_PI);
    }
  }

  auto nTri = this->numberOfTriangles();
  //
  // If triangulation is not available fall back to ray tracing method, unless
  // object is a standard shape, currently Cuboid or Sphere. Should add Cylinder
  // and Cone cases as well.
  //
  if (nTri == 0) {
    double height = 0.0, radius(0.0), innerRadius;
    detail::ShapeInfo::GeometryShape type;
    std::vector<Kernel::V3D> vectors;
    this->GetObjectGeom(type, vectors, innerRadius, radius, height);
    switch (type) {
    case detail::ShapeInfo::GeometryShape::CUBOID:
      std::transform(vectors.begin(), vectors.end(), vectors.begin(),
                     [scaleFactor](const V3D &v) { return v * scaleFactor; });
      return cuboidSolidAngle(observer, vectors);
      break;
    case detail::ShapeInfo::GeometryShape::SPHERE:
      return sphereSolidAngle(observer, vectors, radius);
      break;
    default:
      break;
    }

    //
    // No special case, do the ray trace.
    //
    return rayTraceSolidAngle(observer);
  }
  const auto &vertices = this->getTriangleVertices();
  const auto &faces = this->getTriangleFaces();
  double sangle(0.0), sneg(0.0);
  for (size_t i = 0; i < nTri; i++) {
    int p1 = faces[i * 3], p2 = faces[i * 3 + 1], p3 = faces[i * 3 + 2];
    // would be more efficient to pre-multiply the vertices (copy of) by these
    // factors beforehand
    V3D vp1 = V3D(sx * vertices[3 * p1], sy * vertices[3 * p1 + 1], sz * vertices[3 * p1 + 2]);
    V3D vp2 = V3D(sx * vertices[3 * p2], sy * vertices[3 * p2 + 1], sz * vertices[3 * p2 + 2]);
    V3D vp3 = V3D(sx * vertices[3 * p3], sy * vertices[3 * p3 + 1], sz * vertices[3 * p3 + 2]);
    double sa = triangleSolidAngle(vp1, vp2, vp3, observer);
    if (sa > 0.0)
      sangle += sa;
    else
      sneg += sa;
  }
  return (0.5 * (sangle - sneg));
}

/**
 * For simple shapes, the volume is calculated exactly. For more
 * complex cases, we fall back to Monte Carlo.
 * @return The volume.
 */
double CSGObject::volume() const {
  detail::ShapeInfo::GeometryShape type;
  double height;
  double radius;
  double innerRadius;
  std::vector<Kernel::V3D> vectors;
  this->GetObjectGeom(type, vectors, innerRadius, radius, height);
  switch (type) {
  case detail::ShapeInfo::GeometryShape::CUBOID: {
    // Here, the volume is calculated by the triangular method.
    // We use one of the vertices (vectors[0]) as the reference
    // point.
    double volumeTri = 0.0;
    // Vertices. Name codes follow flb = front-left-bottom etc.
    const Kernel::V3D &flb = vectors[0];
    const Kernel::V3D &flt = vectors[1];
    const Kernel::V3D &frb = vectors[3];
    const Kernel::V3D frt = frb + flt - flb;
    const Kernel::V3D &blb = vectors[2];
    const Kernel::V3D blt = blb + flt - flb;
    const Kernel::V3D brb = blb + frb - flb;
    const Kernel::V3D brt = frt + blb - flb;
    // Normals point out, follow right-handed rule when
    // defining the triangle faces.
    volumeTri += flb.scalar_prod(flt.cross_prod(blb));
    volumeTri += blb.scalar_prod(flt.cross_prod(blt));
    volumeTri += flb.scalar_prod(frb.cross_prod(flt));
    volumeTri += frb.scalar_prod(frt.cross_prod(flt));
    volumeTri += flb.scalar_prod(blb.cross_prod(frb));
    volumeTri += blb.scalar_prod(brb.cross_prod(frb));
    volumeTri += frb.scalar_prod(brb.cross_prod(frt));
    volumeTri += brb.scalar_prod(brt.cross_prod(frt));
    volumeTri += flt.scalar_prod(frt.cross_prod(blt));
    volumeTri += frt.scalar_prod(brt.cross_prod(blt));
    volumeTri += blt.scalar_prod(brt.cross_prod(blb));
    volumeTri += brt.scalar_prod(brb.cross_prod(blb));
    return volumeTri / 6;
  }
  case detail::ShapeInfo::GeometryShape::SPHERE:
    return 4.0 / 3.0 * M_PI * radius * radius * radius;
  case detail::ShapeInfo::GeometryShape::CYLINDER:
    return M_PI * radius * radius * height;
  case detail::ShapeInfo::GeometryShape::HOLLOWCYLINDER:
    return M_PI * height * (radius * radius - innerRadius * innerRadius);
  default:
    // Fall back to Monte Carlo method.
    return monteCarloVolume();
  }
}

/**
 * Calculates the volume of this object by the Monte Carlo method.
 * This method manages singleShotMonteCarloVolume() and uses the
 * standard error as the convergence criteria.
 * @returns The simulated volume of this object.
 */
double CSGObject::monteCarloVolume() const {
  using namespace boost::accumulators;
  const int singleShotIterations = 10000;
  accumulator_set<double, features<tag::mean, tag::error_of<tag::mean>>> accumulate;
  // For seeding the single shot runs.
  std::ranlux48 rnEngine;
  // Warm up statistics.
  for (int i = 0; i < 10; ++i) {
    const auto seed = rnEngine();
    const double volumeMc = singleShotMonteCarloVolume(singleShotIterations, seed);
    accumulate(volumeMc);
  }
  const double relativeErrorTolerance = 1e-3;
  double currentMean;
  double currentError;
  do {
    const auto seed = rnEngine();
    const double volumeMc = singleShotMonteCarloVolume(singleShotIterations, seed);
    accumulate(volumeMc);
    currentMean = mean(accumulate);
    currentError = error_of<tag::mean>(accumulate);
    if (std::isnan(currentError)) {
      currentError = 0;
    }
  } while (currentError / currentMean > relativeErrorTolerance);
  return currentMean;
}

/**
 * Calculates the volume using the Monte Carlo method.
 * @param shotSize Number of iterations.
 * @param seed A number to seed the random number generator.
 * @returns The simulated volume of this object.
 */
double CSGObject::singleShotMonteCarloVolume(const int shotSize, const size_t seed) const {
  const auto &boundingBox = getBoundingBox();
  if (boundingBox.isNull()) {
    throw std::runtime_error("Cannot calculate volume: invalid bounding box.");
  }
  int totalHits = 0;
  const double boundingDx = boundingBox.xMax() - boundingBox.xMin();
  const double boundingDy = boundingBox.yMax() - boundingBox.yMin();
  const double boundingDz = boundingBox.zMax() - boundingBox.zMin();
  PARALLEL {
    const auto threadCount = PARALLEL_NUMBER_OF_THREADS;
    const auto currentThreadNum = PARALLEL_THREAD_NUMBER;
    size_t blocksize = shotSize / threadCount;
    // cppcheck-suppress knownConditionTrueFalse
    if (currentThreadNum == threadCount - 1) {
      // Last thread may have to do threadCount extra iterations in
      // the worst case.
      blocksize = shotSize - (threadCount - 1) * blocksize;
    }
    std::mt19937 rnEngine(static_cast<std::mt19937::result_type>(seed));
    // All threads init their engine with the same seed.
    // We discard the random numbers used by the other threads.
    // This ensures reproducible results independent of the number
    // of threads.
    // We need three random numbers for each iteration.
    rnEngine.discard(currentThreadNum * 3 * blocksize);
    std::uniform_real_distribution<double> rnDistribution(0.0, 1.0);
    int hits = 0;
    for (int i = 0; i < static_cast<int>(blocksize); ++i) {
      double rnd = rnDistribution(rnEngine);
      const double x = boundingBox.xMin() + rnd * boundingDx;
      rnd = rnDistribution(rnEngine);
      const double y = boundingBox.yMin() + rnd * boundingDy;
      rnd = rnDistribution(rnEngine);
      const double z = boundingBox.zMin() + rnd * boundingDz;
      if (isValid(V3D(x, y, z))) {
        ++hits;
      }
    }
    // Collect results.
    PARALLEL_ATOMIC
    totalHits += hits;
  }
  const double ratio = static_cast<double>(totalHits) / static_cast<double>(shotSize);
  const double boundingVolume = boundingDx * boundingDy * boundingDz;
  return ratio * boundingVolume;
}

/**
 * Returns an axis-aligned bounding box that will fit the shape
 * @returns A reference to a bounding box for this shape.
 */
const BoundingBox &CSGObject::getBoundingBox() const {
  // This member function is const given that from a user's perspective it is
  // perfectly reasonable to call it on a const object. We need to call a
  // non-const function in places to update the cache, which is where the
  // const_cast comes into play.

  // If we don't know the extent of the object, the bounding box doesn't mean
  // anything
  if (!m_topRule) {
    const_cast<CSGObject *>(this)->setNullBoundingBox();
    return m_boundingBox;
  }

  // We have a bounding box already, so just return it
  if (m_boundingBox.isNonNull())
    return m_boundingBox;

  // Try to calculate using Rule method first
  const_cast<CSGObject *>(this)->calcBoundingBoxByRule();
  if (m_boundingBox.isNonNull())
    return m_boundingBox;

  // Rule method failed; Try geometric method
  const_cast<CSGObject *>(this)->calcBoundingBoxByGeometry();
  if (m_boundingBox.isNonNull())
    return m_boundingBox;

  // Geometric method failed; try to calculate by vertices
  const_cast<CSGObject *>(this)->calcBoundingBoxByVertices();
  if (m_boundingBox.isNonNull())
    return m_boundingBox;

  // All options failed; give up
  // Set to a large box so that a) we don't keep trying to calculate a box
  // every time this is called and b) to serve as a visual indicator that
  // something went wrong.
  const_cast<CSGObject *>(this)->defineBoundingBox(100, 100, 100, -100, -100, -100);
  return m_boundingBox;
}

/**
 * Attempts to calculate bounding box using Rule system.
 *
 * Stores result in bounding box cache if successful. Will only work for shapes
 * that consist entirely of axis-aligned surfaces and a few special cases (such
 * as Spheres).
 */
void CSGObject::calcBoundingBoxByRule() {
  // Must have a top rule for this to work
  if (!m_topRule)
    return;

  // Set up some unreasonable values that will be refined
  const double huge(1e10);
  const double big(1e4);
  double minX(-huge), minY(-huge), minZ(-huge);
  double maxX(huge), maxY(huge), maxZ(huge);

  // Try to use the Rule system to derive the box
  m_topRule->getBoundingBox(maxX, maxY, maxZ, minX, minY, minZ);

  // Check whether values are reasonable now. Rule system will fail to produce
  // a reasonable box if the shape is not axis-aligned.
  if (minX > -big && maxX < big && minY > -big && maxY < big && minZ > -big && maxZ < big && minX <= maxX &&
      minY <= maxY && minZ <= maxZ) {
    // Values make sense, cache and return bounding box
    defineBoundingBox(maxX, maxY, maxZ, minX, minY, minZ);
  }
}

/**
 * Attempts to calculate bounding box using vertex array.
 *
 * Stores result in bounding box cache if successful. Will only work for shapes
 * that have handlers capable of providing a vertex mesh.
 *
 * @see GeometryHandler::canTriangulate()
 */
void CSGObject::calcBoundingBoxByVertices() {
  // Grab vertex information
  auto vertCount = this->numberOfVertices();

  if (vertCount > 0) {
    const auto &vertArray = this->getTriangleVertices();
    // Unreasonable extents to be overwritten by loop
    constexpr double huge = 1e10;
    double minX, maxX, minY, maxY, minZ, maxZ;
    minX = minY = minZ = huge;
    maxX = maxY = maxZ = -huge;

    // Loop over all vertices and determine minima and maxima on each axis
    for (size_t i = 0; i < vertCount; ++i) {
      auto vx = vertArray[3 * i + 0];
      auto vy = vertArray[3 * i + 1];
      auto vz = vertArray[3 * i + 2];

      minX = std::min(minX, vx);
      maxX = std::max(maxX, vx);
      minY = std::min(minY, vy);
      maxY = std::max(maxY, vy);
      minZ = std::min(minZ, vz);
      maxZ = std::max(maxZ, vz);
    }

    // Store bounding box in cache
    defineBoundingBox(maxX, maxY, maxZ, minX, minY, minZ);
  }
}

/**
 * Attempts to calculate bounding box using object geometry.
 *
 * Stores result in bounding box cache if successful. Will only work for basic
 * shapes that are handled by GluGeometryHandler.
 */
void CSGObject::calcBoundingBoxByGeometry() {
  // Must have a GeometryHandler for this to work
  if (!m_handler)
    return;

  // Extent of bounding box
  double minX, maxX, minY, maxY, minZ, maxZ;

  // Shape geometry data
  detail::ShapeInfo::GeometryShape type;
  std::vector<Kernel::V3D> vectors;
  double radius;
  double height;
  double innerRadius;

  // Will only work for shapes with ShapeInfo
  m_handler->GetObjectGeom(type, vectors, innerRadius, radius, height);
  // Type of shape is given as a simple integer
  switch (type) {
  case detail::ShapeInfo::GeometryShape::CUBOID: {
    // Points as defined in IDF XML
    const auto &lfb = vectors[0]; // Left-Front-Bottom
    const auto &lft = vectors[1]; // Left-Front-Top
    const auto &lbb = vectors[2]; // Left-Back-Bottom
    const auto &rfb = vectors[3]; // Right-Front-Bottom

    // Calculate and add missing corner points to vectors
    auto lbt = lft + (lbb - lfb); // Left-Back-Top
    auto rft = rfb + (lft - lfb); // Right-Front-Top
    auto rbb = lbb + (rfb - lfb); // Right-Back-Bottom
    auto rbt = rbb + (rft - rfb); // Right-Back-Top

    vectors.emplace_back(lbt);
    vectors.emplace_back(rft);
    vectors.emplace_back(rbb);
    vectors.emplace_back(rbt);

    // Unreasonable extents to be replaced by first loop cycle
    constexpr double huge = 1e10;
    minX = minY = minZ = huge;
    maxX = maxY = maxZ = -huge;

    // Loop over all corner points to find minima and maxima on each axis
    for (const auto &vector : vectors) {
      minX = std::min(minX, vector.X());
      maxX = std::max(maxX, vector.X());
      minY = std::min(minY, vector.Y());
      maxY = std::max(maxY, vector.Y());
      minZ = std::min(minZ, vector.Z());
      maxZ = std::max(maxZ, vector.Z());
    }
  } break;
  case detail::ShapeInfo::GeometryShape::HEXAHEDRON: {
    // These will be replaced by more realistic values in the loop below
    minX = minY = minZ = std::numeric_limits<decltype(minZ)>::max();
    maxX = maxY = maxZ = -std::numeric_limits<decltype(maxZ)>::max();

    // Loop over all corner points to find minima and maxima on each axis
    for (const auto &vector : vectors) {
      minX = std::min(minX, vector.X());
      maxX = std::max(maxX, vector.X());
      minY = std::min(minY, vector.Y());
      maxY = std::max(maxY, vector.Y());
      minZ = std::min(minZ, vector.Z());
      maxZ = std::max(maxZ, vector.Z());
    }
  } break;
  case detail::ShapeInfo::GeometryShape::CYLINDER: {
    // Center-point of base and normalized axis based on IDF XML
    const auto &base = vectors[0];
    const auto &axis = vectors[1];
    auto top = base + (axis * height); // Center-point of other end

    // How much of the radius must be considered for each axis
    // (If this ever becomes a performance issue, you could use just the radius
    // for a quick approx that is still guaranteed to fully contain the shape)
    volatile auto rx = radius * sqrt(pow(axis.Y(), 2) + pow(axis.Z(), 2));
    volatile auto ry = radius * sqrt(pow(axis.X(), 2) + pow(axis.Z(), 2));
    volatile auto rz = radius * sqrt(pow(axis.X(), 2) + pow(axis.Y(), 2));

    // The bounding box is drawn around the base and top center-points,
    // then expanded in order to account for the radius
    minX = std::min(base.X(), top.X()) - rx;
    maxX = std::max(base.X(), top.X()) + rx;
    minY = std::min(base.Y(), top.Y()) - ry;
    maxY = std::max(base.Y(), top.Y()) + ry;
    minZ = std::min(base.Z(), top.Z()) - rz;
    maxZ = std::max(base.Z(), top.Z()) + rz;
  } break;

  case detail::ShapeInfo::GeometryShape::CONE: {
    const auto &tip = vectors[0];      // Tip-point of cone
    const auto &axis = vectors[1];     // Normalized axis
    auto base = tip + (axis * height); // Center of base

    // How much of the radius must be considered for each axis
    // (If this ever becomes a performance issue, you could use just the radius
    // for a quick approx that is still guaranteed to fully contain the shape)
    auto rx = radius * sqrt(pow(axis.Y(), 2) + pow(axis.Z(), 2));
    auto ry = radius * sqrt(pow(axis.X(), 2) + pow(axis.Z(), 2));
    auto rz = radius * sqrt(pow(axis.X(), 2) + pow(axis.Y(), 2));

    // For a cone, the adjustment is only applied to the base
    minX = std::min(tip.X(), base.X() - rx);
    maxX = std::max(tip.X(), base.X() + rx);
    minY = std::min(tip.Y(), base.Y() - ry);
    maxY = std::max(tip.Y(), base.Y() + ry);
    minZ = std::min(tip.Z(), base.Z() - rz);
    maxZ = std::max(tip.Z(), base.Z() + rz);
  } break;

  default:  // Invalid (0, -1) or SPHERE (2) which should be handled by Rules
    return; // Don't store bounding box
  }

  // Store bounding box in cache
  defineBoundingBox(maxX, maxY, maxZ, minX, minY, minZ);
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
void CSGObject::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin,
                               double &zmin) const {
  if (!m_topRule) { // If no rule defined then return zero boundbing box
    xmax = ymax = zmax = xmin = ymin = zmin = 0.0;
    return;
  }
  if (!boolBounded) {
    AABBxMax = xmax;
    AABByMax = ymax;
    AABBzMax = zmax;
    AABBxMin = xmin;
    AABByMin = ymin;
    AABBzMin = zmin;
    m_topRule->getBoundingBox(AABBxMax, AABByMax, AABBzMax, AABBxMin, AABByMin, AABBzMin);
    if (AABBxMax >= xmax || AABBxMin <= xmin || AABByMax >= ymax || AABByMin <= ymin || AABBzMax >= zmax ||
        AABBzMin <= zmin)
      boolBounded = false;
    else
      boolBounded = true;
  }
  xmax = AABBxMax;
  ymax = AABByMax;
  zmax = AABBzMax;
  xmin = AABBxMin;
  ymin = AABByMin;
  zmin = AABBzMin;
}

/**
 * Takes input axis aligned bounding box max and min points and stores these as
 *the
 * bounding box for the object. Can be used when getBoundingBox fails and bounds
 *are
 * known.
 *
 * @param xMax :: Maximum value for the bounding box in x direction
 * @param yMax :: Maximum value for the bounding box in y direction
 * @param zMax :: Maximum value for the bounding box in z direction
 * @param xMin :: Minimum value for the bounding box in x direction
 * @param yMin :: Minimum value for the bounding box in y direction
 * @param zMin :: Minimum value for the bounding box in z direction
 */
void CSGObject::defineBoundingBox(const double &xMax, const double &yMax, const double &zMax, const double &xMin,
                                  const double &yMin, const double &zMin) {
  BoundingBox::checkValid(xMax, yMax, zMax, xMin, yMin, zMin);

  AABBxMax = xMax;
  AABByMax = yMax;
  AABBzMax = zMax;
  AABBxMin = xMin;
  AABByMin = yMin;
  AABBzMin = zMin;
  boolBounded = true;

  PARALLEL_CRITICAL(defineBoundingBox) { m_boundingBox = BoundingBox(xMax, yMax, zMax, xMin, yMin, zMin); }
}

/**
 * Set the bounding box to a null box
 */
void CSGObject::setNullBoundingBox() { m_boundingBox = BoundingBox(); }

/**
Try to find a point that lies within (or on) the object
@param[out] point :: on exit set to the point value, if found
@return 1 if point found, 0 otherwise
*/
int CSGObject::getPointInObject(Kernel::V3D &point) const {
  //
  // Simple method - check if origin in object, if not search directions along
  // axes. If that fails, try centre of boundingBox, and paths about there
  //
  Kernel::V3D testPt(0, 0, 0);
  if (searchForObject(testPt)) {
    point = testPt;
    return 1;
  }
  // Try centre of bounding box as initial guess, if we have one.
  const BoundingBox &boundingBox = getBoundingBox();
  if (boundingBox.isNonNull()) {
    testPt = boundingBox.centrePoint();
    if (searchForObject(testPt) > 0) {
      point = testPt;
      return 1;
    }
  }

  return 0;
}

/**
 * Generate a random point within the object.
 * For certain simple shapes, the point is generated directly inside the
 * shape. In the general case, the method simply generates a
 * point within the bounding box and tests if this is a valid point within
 * the object: if so the point is returned otherwise a new point is selected.
 * @param rng  A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @param maxAttempts The maximum number of attempts at generating a point
 * @return whether a point was generated in the object or not
 */
std::optional<Kernel::V3D> CSGObject::generatePointInObject(PseudoRandomNumberGenerator &rng,
                                                            const size_t maxAttempts) const {
  std::optional<V3D> point{std::nullopt};
  // If the shape fills its bounding box well enough then the most efficient
  // way to get the point is just brute force. We'll try that first with
  // just a few attempts.
  // Increasing the brute force attemps speeds up the shapes that fill
  // the bounding box but slows down shapes that leave lots of void
  // within the box. So there is a sweet spot which depends on the actual
  // shape, its dimension and orientation.
  const size_t bruteForceAttempts{std::min(static_cast<size_t>(5), maxAttempts)};
  std::optional<V3D> maybePoint{RandomPoint::inGenericShape(*this, rng, bruteForceAttempts)};
  if (maybePoint) {
    point = maybePoint;
  } else {
    switch (shape()) {
    case detail::ShapeInfo::GeometryShape::CUBOID:
      point = RandomPoint::inCuboid(m_handler->shapeInfo(), rng);
      break;
    case detail::ShapeInfo::GeometryShape::CYLINDER:
      point = RandomPoint::inCylinder(m_handler->shapeInfo(), rng);
      break;
    case detail::ShapeInfo::GeometryShape::HOLLOWCYLINDER:
      point = RandomPoint::inHollowCylinder(m_handler->shapeInfo(), rng);
      break;
    case detail::ShapeInfo::GeometryShape::SPHERE:
      point = RandomPoint::inSphere(m_handler->shapeInfo(), rng);
      break;
    default:
      maybePoint = RandomPoint::inGenericShape(*this, rng, maxAttempts - bruteForceAttempts);
      point = maybePoint;
    }
  }
  return point;
}

/**
 * Generate a random point within the object that is also bound by the
 * activeRegion box.
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @param activeRegion Restrict point generation to this sub-region of the
 * object
 * @param maxAttempts The maximum number of attempts at generating a point
 * @return whether a point was generated in the object or not
 */
std::optional<Kernel::V3D> CSGObject::generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                                            const BoundingBox &activeRegion,
                                                            const size_t maxAttempts) const {
  std::optional<V3D> point{std::nullopt};
  // We'll first try brute force. If the shape fills its bounding box
  // well enough, this should be the fastest method.
  // Increasing the brute force attemps speeds up the shapes that fill
  // the bounding box well but slows down shapes that leave lots of void
  // within the box. So there is a sweet spot which depends on the actual
  // shape, its dimension and orientation.
  const size_t bruteForceAttempts{std::min(static_cast<size_t>(5), maxAttempts)};
  point = RandomPoint::bounded(*this, rng, activeRegion, bruteForceAttempts);
  if (!point) {
    detail::ShapeInfo::GeometryShape shapeGeometry;
    std::vector<Kernel::V3D> shapeVectors;
    double radius;
    double height;
    double innerRadius;
    GetObjectGeom(shapeGeometry, shapeVectors, innerRadius, radius, height);
    switch (shapeGeometry) {
    case detail::ShapeInfo::GeometryShape::CUBOID:
      point = RandomPoint::bounded<RandomPoint::inCuboid>(m_handler->shapeInfo(), rng, activeRegion,
                                                          maxAttempts - bruteForceAttempts);
      break;
    case detail::ShapeInfo::GeometryShape::CYLINDER:
      point = RandomPoint::bounded<RandomPoint::inCylinder>(m_handler->shapeInfo(), rng, activeRegion,
                                                            maxAttempts - bruteForceAttempts);
      break;
    case detail::ShapeInfo::GeometryShape::HOLLOWCYLINDER:
      point = RandomPoint::bounded<RandomPoint::inHollowCylinder>(m_handler->shapeInfo(), rng, activeRegion,
                                                                  maxAttempts - bruteForceAttempts);
      break;
    case detail::ShapeInfo::GeometryShape::SPHERE:
      point = RandomPoint::bounded<RandomPoint::inSphere>(m_handler->shapeInfo(), rng, activeRegion,
                                                          maxAttempts - bruteForceAttempts);
      break;
    default:
      point = RandomPoint::bounded(*this, rng, activeRegion, maxAttempts - bruteForceAttempts);
      break;
    }
  }
  return point;
}

/**
 * Try to find a point that lies within (or on) the object, given a seed point
 * @param point :: on entry the seed point, on exit point in object, if found
 * @return 1 if point found, 0 otherwise
 */
int CSGObject::searchForObject(Kernel::V3D &point) const {
  //
  // Method - check if point in object, if not search directions along
  // principle axes using interceptSurface
  //
  if (isValid(point))
    return 1;
  for (const auto &dir :
       {V3D(1., 0., 0.), V3D(-1., 0., 0.), V3D(0., 1., 0.), V3D(0., -1., 0.), V3D(0., 0., 1.), V3D(0., 0., -1.)}) {
    Geometry::Track tr(point, dir);
    if (this->interceptSurface(tr) > 0) {
      point = tr.cbegin()->entryPoint;
      return 1;
    }
  }
  return 0;
}

/**
 * Set the geometry handler for Object
 * @param[in] h is pointer to the geometry handler.
 */
void CSGObject::setGeometryHandler(const std::shared_ptr<GeometryHandler> &h) {
  if (h)
    m_handler = h;
}

/**
 * Draws the Object using geometry handler, If the handler is not set then this
 * function does nothing.
 */
void CSGObject::draw() const {
  if (m_handler == nullptr)
    return;
  // Render the Object
  m_handler->render();
}

/**
 * Initializes/prepares the object to be rendered, this will generate geometry
 * for object,
 * If the handler is not set then this function does nothing.
 */
void CSGObject::initDraw() const {
  if (m_handler == nullptr)
    return;
  // Render the Object
  m_handler->initialize();
}
/**
 * set vtkGeometryCache writer
 */
void CSGObject::setVtkGeometryCacheWriter(std::shared_ptr<vtkGeometryCacheWriter> writer) {
  vtkCacheWriter = std::move(writer);
  updateGeometryHandler();
}

/**
 * set vtkGeometryCache reader
 */
void CSGObject::setVtkGeometryCacheReader(std::shared_ptr<vtkGeometryCacheReader> reader) {
  vtkCacheReader = std::move(reader);
  updateGeometryHandler();
}

/**
 * Returns the geometry handler
 */
std::shared_ptr<GeometryHandler> CSGObject::getGeometryHandler() const {
  // Check if the geometry handler is upto dated with the cache, if not then
  // cache it now.
  return m_handler;
}

/**
 * Updates the geometry handler if needed
 */
void CSGObject::updateGeometryHandler() {
  if (bGeometryCaching)
    return;
  bGeometryCaching = true;
  // Check if the Geometry handler can be handled for cache
  if (m_handler == nullptr)
    return;
  if (!m_handler->canTriangulate())
    return;
  // Check if the reader exist then read the cache
  if (vtkCacheReader.get() != nullptr) {
    vtkCacheReader->readCacheForObject(this);
  }
  // Check if the writer exist then write the cache
  if (vtkCacheWriter.get() != nullptr) {
    vtkCacheWriter->addObject(this);
  }
}

// Initialize Draw Object

size_t CSGObject::numberOfTriangles() const {
  if (m_handler == nullptr)
    return 0;
  return m_handler->numberOfTriangles();
}
size_t CSGObject::numberOfVertices() const {
  if (m_handler == nullptr)
    return 0;
  return m_handler->numberOfPoints();
}
/**
 * get vertices
 */
const std::vector<double> &CSGObject::getTriangleVertices() const {
  static const std::vector<double> empty;
  if (m_handler == nullptr)
    return empty;
  return m_handler->getTriangleVertices();
}

/**
 * get faces
 */
const std::vector<uint32_t> &CSGObject::getTriangleFaces() const {
  static const std::vector<uint32_t> empty;
  if (m_handler == nullptr)
    return empty;
  return m_handler->getTriangleFaces();
}

detail::ShapeInfo::GeometryShape CSGObject::shape() const {
  if (m_handler && m_handler->hasShapeInfo()) {
    return m_handler->shapeInfo().shape();
  } else {
    return detail::ShapeInfo::GeometryShape::NOSHAPE;
  }
}

const detail::ShapeInfo &CSGObject::shapeInfo() const {
  if (m_handler && m_handler->hasShapeInfo()) {
    return m_handler->shapeInfo();
  } else {
    throw std::logic_error("CSGObject has no ShapeInfo to return");
  }
}

/**
 * get info on standard shapes
 */
void CSGObject::GetObjectGeom(detail::ShapeInfo::GeometryShape &type, std::vector<Kernel::V3D> &vectors,
                              double &innerRadius, double &radius, double &height) const {
  type = detail::ShapeInfo::GeometryShape::NOSHAPE;
  if (m_handler == nullptr)
    return;
  m_handler->GetObjectGeom(type, vectors, innerRadius, radius, height);
}

/** Getter for the shape xml
@return the shape xml.
*/
std::string CSGObject::getShapeXML() const { return this->m_shapeXML; }

} // namespace Mantid::Geometry
