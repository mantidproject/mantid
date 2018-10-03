// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <algorithm>
#include <cmath>
#include <complex>
#include <fstream>
#include <list>
#include <map>
#include <sstream>
#include <stack>
#include <vector>

#include "MantidGeometry/Surfaces/BaseVisit.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Line.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/V3D.h"

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
#include <BRepPrimAPI_MakeCone.hxx>
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("cast-qual")
#endif

namespace Mantid {

namespace Geometry {
using Kernel::Tolerance;
using Kernel::V3D;

// The number of slices to use to approximate a cylinder
int Cone::g_nslices = 10;

// The number of slices to use to approximate a cylinder
int Cone::g_nstacks = 1;

Cone::Cone()
    : Quadratic(), Centre(), Normal(1, 0, 0), alpha(0.0), cangle(1.0)
/**
 Constructor with centre line along X axis
 and centre on origin
 */
{
  setBaseEqn();
}

Cone *Cone::doClone() const
/**
 Makes a clone (implicit virtual copy constructor)
 @return new(*this)
 */
{
  return new Cone(*this);
}

std::unique_ptr<Cone> Cone::clone() const {
  return std::unique_ptr<Cone>(doClone());
}

int Cone::setSurface(const std::string &Pstr)
/**
 Processes a standard MCNPX cone string
 Recall that cones can only be specified on an axis
 Valid input is:
 - k/x cen_x cen_y cen_z radius
 - kx radius
 @return : 0 on success, neg of failure
 */
{
  std::string Line = Pstr;
  std::string item;
  if (!Mantid::Kernel::Strings::section(Line, item) ||
      tolower(item[0]) != 'k' || item.length() < 2 || item.length() > 3)
    return -1;

  // Cones on X/Y/Z axis
  const std::size_t itemPt((item[1] == '/' && item.length() == 3) ? 2 : 1);
  const std::size_t ptype =
      static_cast<std::size_t>(tolower(item[itemPt]) - 'x');
  if (ptype >= 3)
    return -2;
  std::vector<double> norm(3, 0.0);
  std::vector<double> cent(3, 0.0);
  norm[ptype] = 1.0;

  if (itemPt == 1) // kx type cone
  {
    if (!Mantid::Kernel::Strings::section(Line, cent[ptype]))
      return -3;
  } else {
    std::size_t index;
    for (index = 0;
         index < 3 && Mantid::Kernel::Strings::section(Line, cent[index]);
         index++)
      ;
    if (index != 3)
      return -4;
  }
  // The user must enter t^2 which is tan^2(angle) for MCNPX
  double tanAng;
  if (!Mantid::Kernel::Strings::section(Line, tanAng))
    return -5;

  Centre = Kernel::V3D(cent[0], cent[1], cent[2]);
  Normal = Kernel::V3D(norm[0], norm[1], norm[2]);
  setTanAngle(sqrt(tanAng));
  setBaseEqn();
  return 0;
}

int Cone::operator==(const Cone &A) const
/**
 Equality operator. Checks angle,centre and
 normal separately
 @param A :: Cone to compare
 @return A==this within SurfaceTolerance
 */
{
  if (this == &A)
    return 1;
  if (fabs(cangle - A.cangle) > Tolerance)
    return 0;
  if (Centre.distance(A.Centre) > Tolerance)
    return 0;
  if (Normal.distance(A.Normal) > Tolerance)
    return 0;

  return 1;
}

void Cone::setBaseEqn()
/**
 Sets an equation of type
 \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
 */
{
  const double c2(cangle * cangle);
  const double CdotN(Centre.scalar_prod(Normal));
  BaseEqn[0] = c2 - Normal[0] * Normal[0];                      // A x^2
  BaseEqn[1] = c2 - Normal[1] * Normal[1];                      // B y^2
  BaseEqn[2] = c2 - Normal[2] * Normal[2];                      // C z^2
  BaseEqn[3] = -2 * Normal[0] * Normal[1];                      // D xy
  BaseEqn[4] = -2 * Normal[0] * Normal[2];                      // E xz
  BaseEqn[5] = -2 * Normal[1] * Normal[2];                      // F yz
  BaseEqn[6] = 2.0 * (Normal[0] * CdotN - Centre[0] * c2);      // G x
  BaseEqn[7] = 2.0 * (Normal[1] * CdotN - Centre[1] * c2);      // H y
  BaseEqn[8] = 2.0 * (Normal[2] * CdotN - Centre[2] * c2);      // J z
  BaseEqn[9] = c2 * Centre.scalar_prod(Centre) - CdotN * CdotN; // K const
}

void Cone::rotate(const Kernel::Matrix<double> &R)
/**
 Rotate both the centre and the normal direction
 @param R :: Matrix for rotation.
 */
{
  Centre.rotate(R);
  Normal.rotate(R);
  setBaseEqn();
}

void Cone::displace(const Kernel::V3D &A)
/**
 Displace the centre
 Only need to update the centre position
 @param A :: Kernel::V3D to add
 */
{
  Centre += A;
  setBaseEqn();
}

void Cone::setCentre(const Kernel::V3D &A)
/**
 Sets the central point and the Base Equation
 @param A :: New Centre point
 */
{
  Centre = A;
  setBaseEqn();
}

void Cone::setNorm(const Kernel::V3D &A)
/**
 Sets the Normal and the Base Equation
 @param A :: New Normal direction
 */
{
  if (A.norm() > Tolerance) {
    Normal = A;
    Normal.normalize();
    setBaseEqn();
  }
}

void Cone::setAngle(const double A)
/**
 Set the angle of the cone.
 @param A :: Angle in degrees.
 Resets the base equation
 */
{
  alpha = A;
  cangle = cos(M_PI * alpha / 180.0);
  setBaseEqn();
}

void Cone::setTanAngle(const double A)
/**
 Set the cone angle
 Resets the base equation
 @param A :: Tan of the angle  (for MCNPX)
 */
{
  cangle = 1.0 / sqrt(A * A + 1.0); // convert tan(theta) to cos(theta)
  alpha = acos(cangle) * 180.0 / M_PI;
  setBaseEqn();
}

double Cone::distance(const Kernel::V3D &Pt) const
/**
 Calculates the distance from the point to the Cone
 does not calculate the point on the cone that is closest
 @param Pt :: Point to calcuate from

 - normalise to a cone vertex at the origin
 - calculate the angle between the axis and the Point
 - Calculate the distance to P
 @return distance to Pt
 */
{
  const Kernel::V3D Px = Pt - Centre;
  // test is the centre to point distance is zero
  if (Px.norm() < Tolerance)
    return Px.norm();
  double Pangle = Px.scalar_prod(Normal) / Px.norm();
  if (Pangle < 0.0)
    Pangle = acos(-Pangle);
  else
    Pangle = acos(Pangle);

  Pangle -= M_PI * alpha / 180.0;
  return Px.norm() * sin(Pangle);
}

/**
 Calculate if the point R is within
 the cone (return -1) or outside,
 (return 1)
 \todo NEEDS ON SURFACE CALCULATING::
 waiting for a point to cone distance function

 @param R :: Point to determine if in/out of cone
 @return Side of R
 */
int Cone::side(const Kernel::V3D &R) const {

  const Kernel::V3D cR = R - Centre;
  double rptAngle = cR.scalar_prod(Normal);
  rptAngle *= rptAngle / cR.scalar_prod(cR);
  const double eqn(sqrt(rptAngle));
  if (fabs(eqn - cangle) < Tolerance)
    return 0;
  return (eqn > cangle) ? 1 : -1;
}

/**
 Calculate if the point R is on
 the cone (Note: have to be careful here
 since angle calcuation calcuates an angle.
 We need a distance for tolerance!)
 @param R :: Point to check
 @return 1 if on surface and 0 if not not on surface
 */
int Cone::onSurface(const Kernel::V3D &R) const {

  const Kernel::V3D cR = R - Centre;
  double rptAngle = cR.scalar_prod(Normal);
  rptAngle *= rptAngle / cR.scalar_prod(cR);
  const double eqn(sqrt(rptAngle));

  return (fabs(eqn - cangle) > Tolerance) ? 0 : 1;
}

void Cone::write(std::ostream &OX) const
/**
 Write out the cone class in an mcnpx
 format.
 @param OX :: Output Stream (required for multiple std::endl)
 */
{
  //               -3 -2 -1 0 1 2 3
  const char Tailends[] = "zyx xyz";
  const int Ndir = Normal.masterDir(Tolerance);
  if (Ndir == 0) {
    Quadratic::write(OX);
    return;
  }
  std::ostringstream cx;
  Quadratic::writeHeader(cx);

  const int Cdir = Centre.masterDir(Tolerance);
  cx.precision(Surface::Nprecision);
  // Name and transform

  if (Cdir || Centre.nullVector(Tolerance)) {
    cx << " k";
    cx << Tailends[Ndir + 3] << " "; // set x,y,z based on Ndir
    cx << ((Cdir > 0) ? Centre[static_cast<std::size_t>(Cdir - 1)]
                      : Centre[static_cast<std::size_t>(-Cdir - 1)]);
    cx << " ";
  } else {
    cx << " k/";
    cx << Tailends[Ndir + 3] << " "; // set x,y,z based on Ndir
    for (std::size_t i = 0; i < 3; i++)
      cx << Centre[i] << " ";
  }
  const double TA = tan((M_PI * alpha) / 180.0); // tan^2(angle)
  cx << TA * TA;
  Mantid::Kernel::Strings::writeMCNPX(cx.str(), OX);
}

void Cone::getBoundingBox(double &xmax, double &ymax, double &zmax,
                          double &xmin, double &ymin, double &zmin) {
  /**
   Cone bounding box
   Intended to improve bounding box for a general quadratic surface
   Using the surface calculate improved limits on the bounding box, if possible.
   @param xmax :: On input, existing Xmax bound, on exit possibly improved Xmax
   bound
   @param xmin :: On input, existing Xmin bound, on exit possibly improved Xmin
   bound
   @param ymax :: as for xmax
   @param ymin :: as for xmin
   @param zmax :: as for xmax
   @param zmin :: as for xmin
   ///TODO: its bit difficult to find resonable AABB
   ///For now it will return the same bounding box as input
   */
  if (Normal.X() != 0 && Normal.Y() == 0 && Normal.Z() == 0) {
    if (Normal.X() < 0) {
      xmin = std::max(Centre.X(), ymin);
    } else {
      xmax = std::min(Centre.X(), ymax);
    }
    double radius = fabs(xmax - xmin) * sin((M_PI * alpha) / 180.0);
    ymin = Centre.Y() - radius;
    ymax = Centre.Y() + radius;
    zmin = Centre.Z() - radius;
    zmax = Centre.Z() + radius;
  } else if (Normal.X() == 0 && Normal.Y() != 0 && Normal.Z() == 0) {
    if (Normal.Y() < 0) {
      ymin = std::max(Centre.Y(), ymin);
    } else {
      ymax = std::min(Centre.Y(), ymax);
    }
    double radius = fabs(ymax - ymin) * sin((M_PI * alpha) / 180.0);
    xmin = Centre.X() - radius;
    xmax = Centre.X() + radius;
    zmin = Centre.Z() - radius;
    zmax = Centre.Z() + radius;
  } else if (Normal.X() == 0 && Normal.Y() == 0 && Normal.Z() != 0) {
    if (Normal.Z() < 0) {
      zmin = std::max(Centre.Z(), ymin);
    } else {
      zmax = std::min(Centre.Z(), ymax);
    }
    double radius = fabs(zmax - zmin) * sin((M_PI * alpha) / 180.0);
    xmin = Centre.X() - radius;
    xmax = Centre.X() + radius;
    ymin = Centre.Y() - radius;
    ymax = Centre.Y() + radius;
  }
}

#ifdef ENABLE_OPENCASCADE
TopoDS_Shape Cone::createShape() {
  gp_Ax2 gpA(gp_Pnt(Centre[0], Centre[1], Centre[2]),
             gp_Dir(Normal[0], Normal[1], Normal[2]));
  return BRepPrimAPI_MakeCone(gpA, 0.0,
                              1000.0 / tan(acos(cangle * M_PI / 180.0)), 1000.0,
                              2.0 * M_PI)
      .Shape();
}
#endif
} // NAMESPACE Geometry

} // NAMESPACE Mantid
