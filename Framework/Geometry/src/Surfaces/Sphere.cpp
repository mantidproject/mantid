// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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

namespace Mantid {

namespace Geometry {
using Kernel::Tolerance;
using Kernel::V3D;

// The number of slices to use to approximate a sphere
int Sphere::g_nslices = 5;

// The number of slices to use to approximate a sphere
int Sphere::g_nstacks = 5;

Sphere::Sphere()
    : Quadratic(), Centre(0, 0, 0), Radius(0.0)
/**
Default constructor
make sphere at the origin radius zero
*/
{
  setBaseEqn();
}

Sphere *Sphere::doClone() const
/**
Makes a clone (implicit virtual copy constructor)
@return new (*this)
*/
{
  return new Sphere(*this);
}

std::unique_ptr<Sphere> Sphere::clone() const
/**
Makes a clone (implicit virtual copy constructor)
@return new (*this)
*/
{
  return std::unique_ptr<Sphere>(doClone());
}

int Sphere::setSurface(const std::string &Pstr)
/**
Processes a standard MCNPX cone string
Recall that cones can only be specified on an axis
Valid input is:
- so radius
- s cen_x cen_y cen_z radius
- sx - cen_x radius
@return : 0 on success, neg of failure
*/
{
  std::string Line = Pstr;
  std::string item;
  if (!Mantid::Kernel::Strings::section(Line, item) ||
      tolower(item[0]) != 's' || item.length() > 2)
    return -1;

  std::vector<double> cent(3, 0.0);
  double R;
  if (item.length() == 2) // sx/sy/sz
  {
    if (tolower(item[1]) != 'o') {
      const std::size_t pType =
          static_cast<std::size_t>(tolower(item[1]) - 'x');
      if (pType > 2)
        return -3;
      if (!Mantid::Kernel::Strings::section(Line, cent[pType]))
        return -4;
    }
  } else if (item.length() == 1) {
    std::size_t index;
    for (index = 0;
         index < 3 && Mantid::Kernel::Strings::section(Line, cent[index]);
         index++)
      ;
    if (index != 3)
      return -5;
  } else
    return -6;
  if (!Mantid::Kernel::Strings::section(Line, R))
    return -7;

  Centre = Kernel::V3D(cent[0], cent[1], cent[2]);
  Radius = R;
  setBaseEqn();
  return 0;
}

int Sphere::side(const Kernel::V3D &Pt) const
/**
Calculate where the point Pt is relative to the
sphere.
@param Pt :: Point to test
@retval -1 :: Pt within sphere
@retval 0 :: point on the surface (within CTolerance)
@retval 1 :: Pt outside the sphere
*/
{
  // MG:  Surface test  - This does not use onSurface since it would double the
  // amount of
  // computation if the object is not on the surface which is most likely
  const double xdiff(Pt.X() - Centre.X()), ydiff(Pt.Y() - Centre.Y()),
      zdiff(Pt.Z() - Centre.Z());
  const double displace =
      sqrt(xdiff * xdiff + ydiff * ydiff + zdiff * zdiff) - Radius;
  if (fabs(displace) < Tolerance) {
    return 0;
  }
  return (displace > 0.0) ? 1 : -1;
}

int Sphere::onSurface(const Kernel::V3D &Pt) const
/**
Calculate if the point Pt on the surface of the sphere
(within tolerance CTolerance)
@param Pt :: Point to check
@return 1 :: on the surfacae or 0 if not.
*/
{
  if (distance(Pt) > Tolerance) {
    return 0;
  }
  return 1;
}

double Sphere::distance(const Kernel::V3D &Pt) const
/**
Determine the shortest distance from the Surface
to the Point.
@param Pt :: Point to calculate distance from
@return distance (Positive only)
*/
{
  const Kernel::V3D disp_vec = Pt - Centre;
  return std::abs(disp_vec.norm() - Radius);
}

void Sphere::displace(const Kernel::V3D &Pt)
/**
Apply a shift of the centre
@param Pt :: distance to add to the centre
*/
{
  Centre += Pt;
  Quadratic::displace(Pt);
}

void Sphere::rotate(const Kernel::Matrix<double> &MA)
/**
Apply a Rotation matrix
@param MA :: matrix to rotate by
*/
{
  Centre.rotate(MA);
  Quadratic::rotate(MA);
}

double Sphere::centreToPoint(const V3D &pt) const {
  /**
  Compute the distance between the given point and the centre of the sphere
  @param pt :: The chosen point
  */
  const Kernel::V3D displace_vec = pt - Centre;
  return displace_vec.norm();
}

void Sphere::setCentre(const Kernel::V3D &A)
/**
Set the centre point
@param A :: New Centre Point
*/
{
  Centre = A;
  setBaseEqn();
}

void Sphere::setBaseEqn()
/**
Sets an equation of type (general sphere)
\f[ x^2+y^2+z^2+Gx+Hy+Jz+K=0 \f]
*/
{
  BaseEqn[0] = 1.0;                                          // A x^2
  BaseEqn[1] = 1.0;                                          // B y^2
  BaseEqn[2] = 1.0;                                          // C z^2
  BaseEqn[3] = 0.0;                                          // D xy
  BaseEqn[4] = 0.0;                                          // E xz
  BaseEqn[5] = 0.0;                                          // F yz
  BaseEqn[6] = -2.0 * Centre[0];                             // G x
  BaseEqn[7] = -2.0 * Centre[1];                             // H y
  BaseEqn[8] = -2.0 * Centre[2];                             // J z
  BaseEqn[9] = Centre.scalar_prod(Centre) - Radius * Radius; // K const
}

void Sphere::write(std::ostream &OX) const
/**
Object of write is to output a MCNPX plane info
@param OX :: Output stream (required for multiple std::endl)
\todo (Needs precision)
*/
{
  std::ostringstream cx;
  Quadratic::writeHeader(cx);
  cx.precision(Surface::Nprecision);
  if (Centre.distance(Kernel::V3D(0, 0, 0)) < Tolerance) {
    cx << "so " << Radius;
  } else {
    cx << "s " << Centre << " " << Radius;
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
void Sphere::getBoundingBox(double &xmax, double &ymax, double &zmax,
                            double &xmin, double &ymin, double &zmin) {
  xmax = Centre[0] + Radius;
  ymax = Centre[1] + Radius;
  zmax = Centre[2] + Radius;
  xmin = Centre[0] - Radius;
  ymin = Centre[1] - Radius;
  zmin = Centre[2] - Radius;
}

#ifdef ENABLE_OPENCASCADE
TopoDS_Shape Sphere::createShape() {
  return BRepPrimAPI_MakeSphere(gp_Pnt(Centre[0], Centre[1], Centre[2]), Radius)
      .Shape();
}
#endif

} // NAMESPACE Geometry

} // NAMESPACE Mantid
