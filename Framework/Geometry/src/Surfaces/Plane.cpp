// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Tolerance.h"

#include <limits>

#ifdef ENABLE_OPENCASCADE
// Opencascade defines _USE_MATH_DEFINES without checking whether it is already
// used.
// Undefine it here before we include the headers to avoid a warning
#ifdef _MSC_VER
#undef _USE_MATH_DEFINES
#ifdef M_SQRT1_2
#undef M_SQRT1_2
#endif
#endif

#include "MantidKernel/WarningSuppressions.h"
GNU_DIAG_OFF("conversion")
GNU_DIAG_OFF("cast-qual")
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <gp_Pln.hxx>
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("cast-qual")
#endif

namespace Mantid::Geometry {
namespace {
Kernel::Logger logger("Plane");
}
using Kernel::Tolerance;
using Kernel::V3D;

Plane::Plane()
    : Quadratic(), m_normVec(1.0, 0.0, 0.0), m_distance(0)
/**
  Constructor: sets plane in y-z plane and throught origin
*/
{
  setBaseEqn();
}

Plane *Plane::doClone() const
/**
  Makes a clone (implicit virtual copy constructor)
  @return new(this)
*/
{
  return new Plane(*this);
}

std::unique_ptr<Plane> Plane::clone() const
/**
 Makes a clone (implicit virtual copy constructor)
 @return new(this)
 */
{
  return std::unique_ptr<Plane>(doClone());
}

int Plane::setSurface(const std::string &Pstr)
/**
   processes a standard MCNPX plane string:
   There are three types :
   - (A) px Distance
   - (B) p A B C D (equation Ax+By+Cz=D)
   - (C) p V3D V3D V3D
   @param Pstr :: String to make into a plane of type p{xyz} or p
   @return 0 on success, -ve of failure
*/
{
  // Two types of plane string p[x-z]  and p
  std::string Line = Pstr;
  std::string item;

  if (!Mantid::Kernel::Strings::section(Line, item) || tolower(item[0]) != 'p')
    return -1;
  // Only 3 need to be declared
  double surf[9] = {0.0, 0, 0, 0, 0};

  if (item.size() == 1) // PROCESS BASIC PLANE:
  {
    int cnt;
    for (cnt = 0; cnt < 9 && Mantid::Kernel::Strings::section(Line, surf[cnt]); cnt++)
      ;

    if (cnt != 4 && cnt != 9)
      return -3;
    if (cnt == 9) // V3D type
    {
      Kernel::V3D A = Kernel::V3D(surf[0], surf[1], surf[2]);
      Kernel::V3D B = Kernel::V3D(surf[3], surf[4], surf[5]);
      Kernel::V3D C = Kernel::V3D(surf[6], surf[7], surf[8]);
      B -= A;
      C -= A;
      m_normVec = B * C;
      m_normVec.normalize();
      m_distance = A.scalar_prod(m_normVec);
    } else // Norm Equation:
    {
      m_normVec = Kernel::V3D(surf[0], surf[1], surf[2]);
      const double ll = m_normVec.normalize();
      if (ll < Tolerance) // avoid
        return -4;
      m_distance = surf[3] / ll;
    }
  } else if (item.size() == 2) //  PROCESS px type PLANE
  {
    const auto ptype = static_cast<int>(tolower(item[1]) - 'x');
    if (ptype < 0 || ptype > 2) // Not x,y,z
      return -5;
    surf[ptype] = 1.0;
    if (!Mantid::Kernel::Strings::convert(Line, m_distance))
      return -6; // Too short or no number
    m_normVec = Kernel::V3D(surf[0], surf[1], surf[2]);
  } else
    return -3; // WRONG NAME

  setBaseEqn();
  return 0;
}

int Plane::setPlane(const Kernel::V3D &P, const Kernel::V3D &N)
/**
  Given a point and a normal direction set the plane
  @param P :: Point for plane to pass thought
  @param N :: Normal for the plane
  @retval 0 :: success
*/
{
  try {
    m_normVec = normalize(N);
  } catch (std::runtime_error &) {
    throw std::invalid_argument("Attempt to create Plane with zero normal");
  }
  m_distance = P.scalar_prod(m_normVec);
  setBaseEqn();
  return 0;
}

void Plane::rotate(const Kernel::Matrix<double> &MA)
/**
  Rotate the plane about the origin by MA
  @param MA :: direct rotation matrix (3x3)
*/
{
  m_normVec.rotate(MA);
  m_normVec.normalize();
  Quadratic::rotate(MA);
}

void Plane::displace(const Kernel::V3D &Sp)
/**
  Displace the plane by Point Sp.
  i.e. r+sp now on the plane
  @param Sp :: point value of displacement
*/
{
  m_distance += m_normVec.scalar_prod(Sp);
  Quadratic::displace(Sp);
}

double Plane::distance(const Kernel::V3D &A) const
/**
  Determine the distance of point A from the plane
  returns a value relative to the normal
  @param A :: point to get distance from
  @return singed distance from point
*/
{
  return A.scalar_prod(m_normVec) - m_distance;
}

double Plane::dotProd(const Plane &A) const
/**
  @param A :: plane to calculate the normal distance from x
  @return the Normal.A.Normal dot product
*/
{
  return m_normVec.scalar_prod(A.m_normVec);
}

Kernel::V3D Plane::crossProd(const Plane &A) const
/**
  Take the cross produce of the normals
  @param A :: plane to calculate the cross product from
  @return the Normal x A.Normal cross product
*/
{
  return m_normVec.cross_prod(A.m_normVec);
}

int Plane::side(const Kernel::V3D &A) const
/**
  Calcualates the side that the point is on
  @param A :: test point
  @retval +ve :: on the same side as the normal
  @retval -ve :: the  opposite side
  @retval 0 :: A is on the plane itself (within tolerence)
*/
{
  const double Dp = m_normVec.scalar_prod(A) - m_distance;
  if (Tolerance < std::abs(Dp))
    return (Dp > 0) ? 1 : -1;
  return 0;
}

bool Plane::onSurface(const Kernel::V3D &A) const
/**
   Calcuate the side that the point is on
   and returns success if it is on the surface.
   - Uses getSurfaceTolerance to determine the closeness
*/
{
  return (side(A) == 0);
}

void Plane::print() const
/**
  Prints out the surface info and
  the Plane info.
*/
{
  Quadratic::print();
  logger.debug() << "m_normVec == " << m_normVec << " : " << m_distance << '\n';
}

std::size_t Plane::planeType() const
/**
   Find if the normal vector allows it to be a special
   type of plane (x,y,z direction)
   (Assumes m_normVec is a unit vector)
   @retval 1-3 :: on the x,y,z axis
   @retval 0 :: general plane
*/
{
  for (std::size_t i = 0; i < 3; i++)
    if (fabs(m_normVec[i]) > (1.0 - Tolerance))
      return i + 1;
  return 0;
}

/**
 *   Sets the general equation for a plane
 */
void Plane::setBaseEqn() {
  BaseEqn[0] = 0.0;          // A x^2
  BaseEqn[1] = 0.0;          // B y^2
  BaseEqn[2] = 0.0;          // C z^2
  BaseEqn[3] = 0.0;          // D xy
  BaseEqn[4] = 0.0;          // E xz
  BaseEqn[5] = 0.0;          // F yz
  BaseEqn[6] = m_normVec[0]; // G x
  BaseEqn[7] = m_normVec[1]; // H y
  BaseEqn[8] = m_normVec[2]; // J z
  BaseEqn[9] = -m_distance;  // K const
}

/**
 *   Object of write is to output a MCNPX plane info
 *   @param OX :: Output stream (required for multiple std::endl)
 *   @todo (Needs precision)
 */
void Plane::write(std::ostream &OX) const {
  std::ostringstream cx;
  Surface::writeHeader(cx);
  cx.precision(Surface::Nprecision);
  const std::size_t ptype = planeType();
  if (!ptype)
    cx << "p " << m_normVec[0] << " " << m_normVec[1] << " " << m_normVec[2] << " " << m_distance;
  else if (m_normVec[ptype - 1] < 0)
    cx << "p"
       << "xyz"[ptype - 1] << " " << -m_distance;
  else
    cx << "p"
       << "xyz"[ptype - 1] << " " << m_distance;

  Mantid::Kernel::Strings::writeMCNPX(cx.str(), OX);
}

/**
 * Returns the point of intersection of line with the plane
 * @param startpt :: input start point of the line
 * @param endpt :: input end point of the line
 * @param output :: output point of intersection
 * @return The number of points of intersection
 */
int Plane::LineIntersectionWithPlane(V3D startpt, V3D endpt, V3D &output) {
  double const sprod = this->getNormal().scalar_prod(startpt - endpt);
  if (sprod == 0)
    return 0;
  double const projection = m_normVec[0] * startpt[0] + m_normVec[1] * startpt[1] + m_normVec[2] * startpt[2];
  double s1 = (projection - m_distance) / sprod;
  if (s1 < 0 || s1 > 1)
    return 0;
  // The expressions below for resolving the point of intersection are
  // resilient to the corner m_distance << sprod.
  double const ratio = projection / sprod;
  output[0] = ratio * endpt[0] + (1 - ratio) * startpt[0] - ((endpt[0] - startpt[0]) / sprod) * m_distance;
  output[1] = ratio * endpt[1] + (1 - ratio) * startpt[1] - ((endpt[1] - startpt[1]) / sprod) * m_distance;
  output[2] = ratio * endpt[2] + (1 - ratio) * startpt[2] - ((endpt[2] - startpt[2]) / sprod) * m_distance;
  return 1;
}

/**
 * Returns the bounding box values for plane, double max is infinity and double
 * min is -infinity
 * A very crude way of finding the bounding box but its very fast.
 * @param xmax :: input & output maximum value in x direction
 * @param ymax :: input & output maximum value in y direction
 * @param zmax :: input & output maximum value in z direction
 * @param xmin :: input & output minimum value in x direction
 * @param ymin :: input & output minimum value in y direction
 * @param zmin :: input & output minimum value in z direction
 */
void Plane::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) {
  // to get the bounding box calculate the normal and the starting point
  V3D vertex1(xmin, ymin, zmin);
  V3D vertex2(xmax, ymin, zmin);
  V3D vertex3(xmax, ymax, zmin);
  V3D vertex4(xmin, ymax, zmin);
  V3D vertex5(xmin, ymin, zmax);
  V3D vertex6(xmax, ymin, zmax);
  V3D vertex7(xmax, ymax, zmax);
  V3D vertex8(xmin, ymax, zmax);
  // find which points lie on which side of the plane
  // find where the plane cuts the cube
  //(xmin,ymin,zmin)--- (xmax,ymin,zmin)   1
  //(xmax,ymin,zmin)--- (xmax,ymin,zmax)   2
  //(xmax,ymin,zmax)--- (xmin,ymin,zmax)   3
  //(xmin,ymin,zmax)--- (xmin,ymin,zmin)   4
  //(xmin,ymax,zmin)--- (xmax,ymax,zmin)   5
  //(xmax,ymax,zmin)--- (xmax,ymax,zmax)   6
  //(xmax,ymax,zmax)--- (xmin,ymax,zmax)   7
  //(xmin,ymax,zmax)--- (xmin,ymax,zmin)   8
  //(xmin,ymin,zmin)--- (xmin,ymax,zmin)   9
  //(xmax,ymin,zmin)--- (xmax,ymax,zmin)  10
  //(xmax,ymin,zmax)--- (xmax,ymax,zmax)  11
  //(xmin,ymin,zmax)--- (xmin,ymax,zmax)  12
  std::vector<V3D> listOfPoints;
  if (this->side(vertex1) <= 0)
    listOfPoints.emplace_back(vertex1);
  if (this->side(vertex2) <= 0)
    listOfPoints.emplace_back(vertex2);
  if (this->side(vertex3) <= 0)
    listOfPoints.emplace_back(vertex3);
  if (this->side(vertex4) <= 0)
    listOfPoints.emplace_back(vertex4);
  if (this->side(vertex5) <= 0)
    listOfPoints.emplace_back(vertex5);
  if (this->side(vertex6) <= 0)
    listOfPoints.emplace_back(vertex6);
  if (this->side(vertex7) <= 0)
    listOfPoints.emplace_back(vertex7);
  if (this->side(vertex8) <= 0)
    listOfPoints.emplace_back(vertex8);
  V3D edge1, edge2, edge3, edge4, edge5, edge6, edge7, edge8, edge9, edge10, edge11, edge12;
  if (LineIntersectionWithPlane(vertex1, vertex2, edge1) == 1)
    listOfPoints.emplace_back(edge1);
  if (LineIntersectionWithPlane(vertex2, vertex3, edge2) == 1)
    listOfPoints.emplace_back(edge2);
  if (LineIntersectionWithPlane(vertex3, vertex4, edge3) == 1)
    listOfPoints.emplace_back(edge3);
  if (LineIntersectionWithPlane(vertex4, vertex1, edge4) == 1)
    listOfPoints.emplace_back(edge4);
  if (LineIntersectionWithPlane(vertex5, vertex6, edge5) == 1)
    listOfPoints.emplace_back(edge5);
  if (LineIntersectionWithPlane(vertex6, vertex7, edge6) == 1)
    listOfPoints.emplace_back(edge6);
  if (LineIntersectionWithPlane(vertex7, vertex8, edge7) == 1)
    listOfPoints.emplace_back(edge7);
  if (LineIntersectionWithPlane(vertex8, vertex5, edge8) == 1)
    listOfPoints.emplace_back(edge8);
  if (LineIntersectionWithPlane(vertex1, vertex5, edge9) == 1)
    listOfPoints.emplace_back(edge9);
  if (LineIntersectionWithPlane(vertex2, vertex6, edge10) == 1)
    listOfPoints.emplace_back(edge10);
  if (LineIntersectionWithPlane(vertex3, vertex7, edge11) == 1)
    listOfPoints.emplace_back(edge11);
  if (LineIntersectionWithPlane(vertex4, vertex8, edge12) == 1)
    listOfPoints.emplace_back(edge12);
  // now sort the vertices to find the  mins and max
  //	std::cout<<listOfPoints.size()<<'\n';
  if (!listOfPoints.empty()) {
    xmin = ymin = zmin = std::numeric_limits<double>::max();
    xmax = ymax = zmax = std::numeric_limits<double>::lowest();
    for (std::vector<V3D>::const_iterator it = listOfPoints.begin(); it != listOfPoints.end(); ++it) {
      //			std::cout<<(*it)<<'\n';
      if ((*it)[0] < xmin)
        xmin = (*it)[0];
      if ((*it)[1] < ymin)
        ymin = (*it)[1];
      if ((*it)[2] < zmin)
        zmin = (*it)[2];
      if ((*it)[0] > xmax)
        xmax = (*it)[0];
      if ((*it)[1] > ymax)
        ymax = (*it)[1];
      if ((*it)[2] > zmax)
        zmax = (*it)[2];
    }
  }
}

#ifdef ENABLE_OPENCASCADE
TopoDS_Shape Plane::createShape() {
  // Get Plane normal and distance.
  V3D normal = this->getNormal();
  double norm2 = normal.norm2();
  if (norm2 == 0.0) {
    throw std::runtime_error("Cannot create a plane with zero normal");
  }
  double distanceFromOrigin = this->getDistance();
  // Find point closest to origin
  double t = distanceFromOrigin / norm2;
  // Create Half Space
  TopoDS_Face P = BRepBuilderAPI_MakeFace(gp_Pln(normal[0], normal[1], normal[2], -distanceFromOrigin)).Face();

  TopoDS_Shape Result =
      BRepPrimAPI_MakeHalfSpace(P, gp_Pnt(normal[0] * (1 + t), normal[1] * (1 + t), normal[2] * (1 + t))).Solid();
  return Result.Complemented();
}
#endif

} // namespace Mantid::Geometry
