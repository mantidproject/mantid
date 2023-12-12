// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Surfaces/Cylinder.h"
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
#include <BRepPrimAPI_MakeCylinder.hxx>
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("cast-qual")
#endif

namespace Mantid::Geometry {
namespace {
Kernel::Logger logger("Cylinder");
}
using Kernel::Tolerance;
using Kernel::V3D;

Cylinder::Cylinder()
    : Quadratic(), m_centre(), m_normal(1, 0, 0), m_normVec(0), m_radius(0.0)
/**
 Standard Constructor creats a cylinder (radius 0)
 along the x axis
 */
{
  // Called after it has been sized by Quadratic
  Cylinder::setBaseEqn();
}

Cylinder *Cylinder::doClone() const
/**
 Makes a clone (implicit virtual copy constructor)
 @return Copy(*this)
 */
{
  return new Cylinder(*this);
}

std::unique_ptr<Cylinder> Cylinder::clone() const
/**
Makes a clone (implicit virtual copy constructor)
@return Copy(*this)
*/
{
  return std::unique_ptr<Cylinder>(doClone());
}

int Cylinder::setSurface(const std::string &Pstr)
/**
 Processes a standard MCNPX cone string
 Recall that cones can only be specified on an axis
 Valid input is:
 - c/x cen_y cen_z radius
 - cx radius
 @param Pstr :: Input string
 @return : 0 on success, neg of failure
 */
{
  enum { errDesc = -1, errAxis = -2, errCent = -3, errRadius = -4 };

  std::string Line = Pstr;
  std::string item;
  if (!Mantid::Kernel::Strings::section(Line, item) || tolower(item[0]) != 'c' || item.length() < 2 ||
      item.length() > 3)
    return errDesc;

  // Cylinders on X/Y/Z axis
  const std::size_t itemPt((item[1] == '/' && item.length() == 3) ? 2 : 1);
  const auto ptype = static_cast<std::size_t>(tolower(item[itemPt]) - 'x');
  if (ptype >= 3)
    return errAxis;
  std::vector<double> norm(3, 0.0);
  std::vector<double> cent(3, 0.0);
  norm[ptype] = 1.0;

  if (itemPt != 1) {
    // get the other two coordinates
    std::size_t index((!ptype) ? 1 : 0);
    while (index < 3 && Mantid::Kernel::Strings::section(Line, cent[index])) {
      index++;
      if (index == ptype)
        index++;
    }
    if (index != 3)
      return errCent;
  }
  // Now get radius
  double R;
  if (!Mantid::Kernel::Strings::section(Line, R) || R <= 0.0)
    return errRadius;

  m_centre = Kernel::V3D(cent[0], cent[1], cent[2]);
  m_normal = Kernel::V3D(norm[0], norm[1], norm[2]);
  m_normVec = ptype + 1;
  setRadiusInternal(R);
  setBaseEqn();
  return 0;
}

int Cylinder::side(const Kernel::V3D &Pt) const
/**
 Calculate if the point PT within the middle
 of the cylinder
 @param Pt :: Point to check
 @retval -1 :: within cylinder
 @retval 1 :: outside the cylinder
 @retval 0 :: on the surface
 */
{
  if (m_normVec) // m_normVec =1-3 (point to exclude == m_normVec-1)
  {
    if (m_radius > 0.0) {
      double x = Pt[m_normVec % 3] - m_centre[m_normVec % 3];
      x *= x;
      double y = Pt[(m_normVec + 1) % 3] - m_centre[(m_normVec + 1) % 3];
      y *= y;
      double displacement = x + y - m_radius * m_radius;
      if (fabs(displacement * m_oneoverradius) < Tolerance)
        return 0;
      return (displacement > 0.0) ? 1 : -1;
    } else {
      return -1;
    }
  }
  return Quadratic::side(Pt);
}

bool Cylinder::onSurface(const Kernel::V3D &Pt) const
/**
 Calculate if the point PT on the cylinder
 @param Pt :: Kernel::V3D to test
 */
{
  if (m_normVec > 0) // m_normVec =1-3 (point to exclude == m_normVec-1)
  {
    double x = Pt[m_normVec % 3] - m_centre[m_normVec % 3];
    x *= x;
    double y = Pt[(m_normVec + 1) % 3] - m_centre[(m_normVec + 1) % 3];
    y *= y;
    return (std::abs((x + y) - m_radius * m_radius) <= Tolerance);
  }
  return Quadratic::onSurface(Pt);
}

void Cylinder::setNormVec()
/**
 Find if the normal vector allows it to be a special
 type of cylinder on the x,y or z axis
 */
{
  m_normVec = 0;
  for (std::size_t i = 0; i < 3; i++) {
    if (fabs(m_normal[i]) > (1.0 - Tolerance)) {
      m_normVec = i + 1;
      return;
    }
  }
}

void Cylinder::rotate(const Kernel::Matrix<double> &MA)
/**
 Apply a rotation to the cylinder and re-check the
 status of the main axis.
 @param MA :: Rotation Matrix (not inverted)
 */
{
  m_centre.rotate(MA);
  m_normal.rotate(MA);
  m_normal.normalize();
  setNormVec();
  Quadratic::rotate(MA);
}

void Cylinder::displace(const Kernel::V3D &Pt)
/**
 Apply a displacement Pt
 @param Pt :: Displacement to add to the centre
 */
{
  if (m_normVec > 0) {
    m_centre[m_normVec % 3] += Pt[m_normVec % 3];
    m_centre[(m_normVec + 1) % 3] += Pt[(m_normVec + 1) % 3];
  } else
    m_centre += Pt;
  Quadratic::displace(Pt);
}

void Cylinder::setCentre(const Kernel::V3D &A)
/**
 Sets the centre Kernel::V3D
 @param A :: centre point
 */
{
  m_centre = A;
  setBaseEqn();
}

void Cylinder::setNorm(const Kernel::V3D &A)
/**
 Sets the centre line unit vector
 A does not need to be a unit vector
 @param A :: Vector along the centre line
 */
{
  m_normal = normalize(A);
  setBaseEqn();
  setNormVec();
}

void Cylinder::setBaseEqn()
/**
 Sets an equation of type (cylinder)
 \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
 */
{
  const double CdotN(m_centre.scalar_prod(m_normal));
  BaseEqn[0] = 1.0 - m_normal[0] * m_normal[0];                                      // A x^2
  BaseEqn[1] = 1.0 - m_normal[1] * m_normal[1];                                      // B y^2
  BaseEqn[2] = 1.0 - m_normal[2] * m_normal[2];                                      // C z^2
  BaseEqn[3] = -2 * m_normal[0] * m_normal[1];                                       // D xy
  BaseEqn[4] = -2 * m_normal[0] * m_normal[2];                                       // E xz
  BaseEqn[5] = -2 * m_normal[1] * m_normal[2];                                       // F yz
  BaseEqn[6] = 2.0 * (m_normal[0] * CdotN - m_centre[0]);                            // G x
  BaseEqn[7] = 2.0 * (m_normal[1] * CdotN - m_centre[1]);                            // H y
  BaseEqn[8] = 2.0 * (m_normal[2] * CdotN - m_centre[2]);                            // J z
  BaseEqn[9] = m_centre.scalar_prod(m_centre) - CdotN * CdotN - m_radius * m_radius; // K const
}

double Cylinder::distance(const Kernel::V3D &A) const
/**
 Calculates the distance of point A from
 the surface of the  cylinder.
 @param A :: Point to calculate distance from
 @return :: +ve distance to the surface.

 \todo INCOMPLETE AS Does not deal with the case of
 non axis centre line  (has been updated?? )
 */
{
  // First find the normal going to the point
  const Kernel::V3D Amov = A - m_centre;
  double lambda = Amov.scalar_prod(m_normal);
  const Kernel::V3D Ccut = m_normal * lambda;
  // The distance is from the centre line to the
  return fabs(Ccut.distance(Amov) - m_radius);
}

void Cylinder::write(std::ostream &OX) const
/**
 Write out the cylinder for MCNPX
 @param OX :: output stream
 */
{
  //               -3 -2 -1 0 1 2 3
  const char Tailends[] = "zyx xyz";
  const int Ndir = m_normal.masterDir(Tolerance);
  if (Ndir == 0) {
    // general surface
    Quadratic::write(OX);
    return;
  }

  const int Cdir = m_centre.masterDir(Tolerance);
  std::ostringstream cx;

  writeHeader(cx);
  cx.precision(Surface::Nprecision);
  // Name and transform

  if (Cdir * Cdir == Ndir * Ndir || m_centre.nullVector(Tolerance)) {
    cx << "c";
    cx << Tailends[Ndir + 3] << " "; // set x,y,z based on Ndir
    cx << m_radius;
  } else {
    cx << " c/";
    cx << Tailends[Ndir + 3] << " "; // set x,y,z based on Ndir

    if (Ndir == 1 || Ndir == -1)
      cx << m_centre[1] << " " << m_centre[2] << " ";
    else if (Ndir == 2 || Ndir == -2)
      cx << m_centre[0] << " " << m_centre[2] << " ";
    else
      cx << m_centre[0] << " " << m_centre[1] << " ";

    cx << m_radius;
  }

  Mantid::Kernel::Strings::writeMCNPX(cx.str(), OX);
}

double Cylinder::lineIntersect(const Kernel::V3D &Pt, const Kernel::V3D &uVec) const
/**
 Given a track starting from Pt and traveling along
 uVec determine the intersection point (distance)
 @param Pt :: Point of track start
 @param uVec :: Unit vector of length
 @retval Distance to intersect
 @retval -1 Failed to intersect
 */
{
  (void)Pt;   // Avoid compiler warning
  (void)uVec; // Avoid compiler warning
  return -1;
}

void Cylinder::print() const
/**
 Debug routine to print out basic information
 */
{
  Quadratic::print();
  logger.debug() << "Axis ==" << m_normal << " ";
  logger.debug() << "Centre == " << m_centre << " ";
  logger.debug() << "Radius == " << m_radius << '\n';
}

void Cylinder::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) {
  /**
   Cylinder bounding box
   find the intersection points of the axis of cylinder with the input bounding
   box.
   Using the end points calculate improved limits on the bounding box, if
   possible.
   @param xmax :: On input, existing Xmax bound, on exit possibly improved Xmax
   bound
   @param xmin :: On input, existing Xmin bound, on exit possibly improved Xmin
   bound
   @param ymax :: as for xmax
   @param ymin :: as for xmin
   @param zmax :: as for xmax
   @param zmin :: as for xmin
   */
  std::vector<V3D> listOfPoints;
  double txmax, tymax, tzmax, txmin, tymin, tzmin;
  txmax = xmax;
  tymax = ymax;
  tzmax = zmax;
  txmin = xmin;
  tymin = ymin;
  tzmin = zmin;
  V3D xminPoint, xmaxPoint, yminPoint, ymaxPoint, zminPoint, zmaxPoint;
  // xmin and max plane
  if (m_normal[0] != 0) {
    xminPoint = m_centre + m_normal * ((xmin - m_centre[0]) / m_normal[0]);
    xmaxPoint = m_centre + m_normal * ((xmax - m_centre[0]) / m_normal[0]);
    listOfPoints.emplace_back(xminPoint);
    listOfPoints.emplace_back(xmaxPoint);
  }

  if (m_normal[1] != 0) {
    // ymin plane
    yminPoint = m_centre + m_normal * ((ymin - m_centre[1]) / m_normal[1]);
    // ymax plane
    ymaxPoint = m_centre + m_normal * ((ymax - m_centre[1]) / m_normal[1]);
    listOfPoints.emplace_back(yminPoint);
    listOfPoints.emplace_back(ymaxPoint);
  }
  if (m_normal[2] != 0) {
    // zmin plane
    zminPoint = m_centre + m_normal * ((zmin - m_centre[2]) / m_normal[2]);
    // zmax plane
    zmaxPoint = m_centre + m_normal * ((zmax - m_centre[2]) / m_normal[2]);
    listOfPoints.emplace_back(zminPoint);
    listOfPoints.emplace_back(zmaxPoint);
  }
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
    xmax += m_radius;
    ymax += m_radius;
    zmax += m_radius;
    xmin -= m_radius;
    ymin -= m_radius;
    zmin -= m_radius;
    if (xmax > txmax)
      xmax = txmax;
    if (xmin < txmin)
      xmin = txmin;
    if (ymax > tymax)
      ymax = tymax;
    if (ymin < tymin)
      ymin = tymin;
    if (zmax > tzmax)
      zmax = tzmax;
    if (zmin < tzmin)
      zmin = tzmin;
  }
}

#ifdef ENABLE_OPENCASCADE
TopoDS_Shape Cylinder::createShape() {
  gp_Pnt center;
  center.SetX(m_centre[0] - m_normal[0] * 500.0);
  center.SetY(m_centre[1] - m_normal[1] * 500.0);
  center.SetZ(m_centre[2] - m_normal[2] * 500.0);
  gp_Ax2 gpA(center, gp_Dir(m_normal[0], m_normal[1], m_normal[2]));
  return BRepPrimAPI_MakeCylinder(gpA, m_radius, 1000.0, 2.0 * M_PI).Solid();
}
#endif
} // namespace Mantid::Geometry
