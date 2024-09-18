// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Tolerance.h"

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
#include <BRepPrimAPI_MakeSphere.hxx>
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("cast-qual")
#endif

namespace Mantid::Geometry {
using Kernel::Tolerance;
using Kernel::V3D;

/**
 * Default constructor
 * make sphere at the origin radius zero
 */
Sphere::Sphere() : Sphere({0, 0, 0}, 0.0) {}

/**
 * @brief Sphere::Sphere
 * @param centre
 * @param radius
 */
Sphere::Sphere(const Kernel::V3D &centre, double radius) : Quadratic(), m_centre{centre}, m_radius{radius} {
  setBaseEqn();
}

/**
 * Makes a clone (implicit virtual copy constructor)
 * @return new (*this)
 */
Sphere *Sphere::doClone() const { return new Sphere(*this); }

/**
 * Makes a clone (implicit virtual copy constructor)
 * @return new (*this)
 */
std::unique_ptr<Sphere> Sphere::clone() const { return std::unique_ptr<Sphere>(doClone()); }

/**
 * Processes a standard MCNPX cone string
 * Recall that cones can only be specified on an axis
 * Valid input is:
 * - so radius
 * - s cen_x cen_y cen_z radius
 * - sx - cen_x radius
 * @return : 0 on success, neg of failure
 */
int Sphere::setSurface(const std::string &Pstr) {
  std::string Line = Pstr;
  std::string item;
  if (!Mantid::Kernel::Strings::section(Line, item) || tolower(item[0]) != 's' || item.length() > 2)
    return -1;

  std::vector<double> cent(3, 0.0);
  double R;
  if (item.length() == 2) // sx/sy/sz
  {
    if (tolower(item[1]) != 'o') {
      const auto pType = static_cast<std::size_t>(tolower(item[1]) - 'x');
      if (pType > 2)
        return -3;
      if (!Mantid::Kernel::Strings::section(Line, cent[pType]))
        return -4;
    }
  } else if (item.length() == 1) {
    std::size_t index;
    for (index = 0; index < 3 && Mantid::Kernel::Strings::section(Line, cent[index]); index++)
      ;
    if (index != 3)
      return -5;
  } else
    return -6;
  if (!Mantid::Kernel::Strings::section(Line, R))
    return -7;

  m_centre = Kernel::V3D(cent[0], cent[1], cent[2]);
  m_radius = R;
  setBaseEqn();
  return 0;
}

/**
 * Calculate where the point Pt is relative to the
 * sphere.
 * @param Pt :: Point to test
 * @retval -1 :: Pt within sphere
 * @retval 0 :: point on the surface (within CTolerance)
 * @retval 1 :: Pt outside the sphere
 */
int Sphere::side(const Kernel::V3D &Pt) const

{
  // MG:  Surface test  - This does not use onSurface since it would double the
  // amount of
  // computation if the object is not on the surface which is most likely
  const double xdiff(Pt.X() - m_centre.X()), ydiff(Pt.Y() - m_centre.Y()), zdiff(Pt.Z() - m_centre.Z());
  const double displacement = sqrt(xdiff * xdiff + ydiff * ydiff + zdiff * zdiff) - m_radius;
  if (fabs(displacement) < Tolerance) {
    return 0;
  }
  return (displacement > 0.0) ? 1 : -1;
}

/**
 * Calculate if the point Pt on the surface of the sphere
 * (within tolerance CTolerance)
 * @param Pt :: Point to check
 */
bool Sphere::onSurface(const Kernel::V3D &Pt) const { return (distance(Pt) <= Tolerance); }

/**
 * Determine the shortest distance from the Surface
 * to the Point.
 * @param Pt :: Point to calculate distance from
 * @return distance (Positive only)
 */
double Sphere::distance(const Kernel::V3D &Pt) const {
  const Kernel::V3D disp_vec = Pt - m_centre;
  return std::abs(disp_vec.norm() - m_radius);
}

/**
 * Apply a shift of the centre
 * @param Pt :: distance to add to the centre
 */
void Sphere::displace(const Kernel::V3D &Pt) {
  m_centre += Pt;
  Quadratic::displace(Pt);
}

/**
 * Apply a Rotation matrix
 * @param MA :: matrix to rotate by
 */
void Sphere::rotate(const Kernel::Matrix<double> &MA) {
  m_centre.rotate(MA);
  Quadratic::rotate(MA);
}

/**
 * Compute the distance between the given point and the centre of the sphere
 * @param pt :: The chosen point
 */
double Sphere::centreToPoint(const V3D &pt) const {
  const Kernel::V3D displace_vec = pt - m_centre;
  return displace_vec.norm();
}

/**
 * Set the centre point
 * @param A :: New Centre Point
 */
void Sphere::setCentre(const Kernel::V3D &A) {
  m_centre = A;
  setBaseEqn();
}

/**
 * Sets an equation of type (general sphere)
 * \f[ x^2+y^2+z^2+Gx+Hy+Jz+K=0 \f]
 */
void Sphere::setBaseEqn() {
  BaseEqn[0] = 1.0;                                                  // A x^2
  BaseEqn[1] = 1.0;                                                  // B y^2
  BaseEqn[2] = 1.0;                                                  // C z^2
  BaseEqn[3] = 0.0;                                                  // D xy
  BaseEqn[4] = 0.0;                                                  // E xz
  BaseEqn[5] = 0.0;                                                  // F yz
  BaseEqn[6] = -2.0 * m_centre[0];                                   // G x
  BaseEqn[7] = -2.0 * m_centre[1];                                   // H y
  BaseEqn[8] = -2.0 * m_centre[2];                                   // J z
  BaseEqn[9] = m_centre.scalar_prod(m_centre) - m_radius * m_radius; // K const
}

/**
 * Object of write is to output a MCNPX plane info
 * @param OX :: Output stream (required for multiple std::endl)
 * \todo (Needs precision)
 */
void Sphere::write(std::ostream &OX) const {
  std::ostringstream cx;
  Quadratic::writeHeader(cx);
  cx.precision(Surface::Nprecision);
  if (m_centre.distance(Kernel::V3D(0, 0, 0)) < Tolerance) {
    cx << "so " << m_radius;
  } else {
    cx << "s " << m_centre << " " << m_radius;
  }
  Mantid::Kernel::Strings::writeMCNPX(cx.str(), OX);
}

/**
 * Calculates the bounding box for the sphere and returns the bounding box
 * values.
 * @param xmax :: input and output for the bounding box X axis max value
 * @param ymax :: input and output for the bounding box Y axis max value
 * @param zmax :: input and output for the bounding box Z axis max value
 * @param xmin :: input and output for the bounding box X axis min value
 * @param ymin :: input and output for the bounding box Y axis min value
 * @param zmin :: input and output for the bounding box Z axis min value
 */
void Sphere::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) {
  xmax = m_centre[0] + m_radius;
  ymax = m_centre[1] + m_radius;
  zmax = m_centre[2] + m_radius;
  xmin = m_centre[0] - m_radius;
  ymin = m_centre[1] - m_radius;
  zmin = m_centre[2] - m_radius;
}

#ifdef ENABLE_OPENCASCADE
TopoDS_Shape Sphere::createShape() {
  return BRepPrimAPI_MakeSphere(gp_Pnt(m_centre[0], m_centre[1], m_centre[2]), m_radius).Shape();
}
#endif

} // namespace Mantid::Geometry
