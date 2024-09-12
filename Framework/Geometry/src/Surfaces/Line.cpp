// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Surfaces/Line.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Tolerance.h"
#include <optional>

namespace {
Mantid::Kernel::Logger logger("Line");
}
namespace Mantid::Geometry {

using Kernel::Tolerance;
using Kernel::V3D;

Line::Line()
    : m_origin(), m_direction()
/**
Constructor
*/
{}

Line::Line(const Kernel::V3D &O, const Kernel::V3D &D)
    : m_origin(O), m_direction(normalize(D))
/**
Constructor
*/
{}

Line *Line::clone() const
/**
Virtual copy constructor (not currently used)
@return the cloned line
*/
{
  return new Line(*this);
}

Kernel::V3D Line::getPoint(const double lambda) const
/**
Return the point on the line given lambda*direction
@param lambda :: line position scalar
@return \f$ \vec{O}+ \lambda \vec{D} \f$
*/
{
  return m_origin + m_direction * lambda;
}

double Line::distance(const Kernel::V3D &A) const
/**
Distance of a point from the line
@param A :: test Point
@return absolute distance (not signed)
*/
{
  const double lambda = m_direction.scalar_prod(A - m_origin);
  Kernel::V3D L = getPoint(lambda);
  L -= A;
  return L.norm();
}

int Line::isValid(const Kernel::V3D &A) const
/**
Calculate is point is on line by using distance to determine
if the point is within Tolerance of the line
@param A :: Point to test
@retval 1 : the point is on the line
@retval 0 : Point is not on the line
*/
{
  return (distance(A) > Tolerance) ? 0 : 1;
}

void Line::rotate(const Kernel::Matrix<double> &MA)
/**
Applies the rotation matrix to the
object.
@param MA :: Rotation Matrix
*/
{
  m_origin.rotate(MA);
  m_direction.rotate(MA);
  m_direction.normalize();
}

void Line::displace(const Kernel::V3D &Pt)
/**
Apply a displacement Pt
@param Pt :: Point value of the displacement
*/
{
  m_origin += Pt;
}

int Line::lambdaPair(const int ix, const std::pair<std::complex<double>, std::complex<double>> &SQ, PType &PntOut) const
/**
Helper function to decide which roots to take.
The assumption is that lambda has been solved by quadratic
equation and we require the points that correspond to these
values.
Note: have changed this so that only positive roots are returned.
This makes the quadratic solutions consistent with the ones returned
when asking if a line hits a plane. It is not clear if some other use
cases exist.
@param ix : number of solutions in SQ (0,1,2)
@param SQ : solutions to lambda (the distance along the line
@param PntOut : Output vector of points (added to)
@return Number of real unique points found.
*/
{
  // check results
  if (ix < 1)
    return 0;

  std::optional<Kernel::V3D> Ans1;
  if (SQ.first.imag() == 0.0 && SQ.first.real() >= 0.0) // +ve roots only
  {
    const double lambda = SQ.first.real();
    Ans1 = getPoint(lambda);
    PntOut.emplace_back(*Ans1);
    if (ix < 2) // only one unique root.
      return 1;
  }
  if (SQ.second.imag() == 0.0 && SQ.second.real() >= 0.0) // +ve roots only
  {
    const double lambda = SQ.second.real();
    const Kernel::V3D Ans2 = getPoint(lambda);
    if (!Ans1) {
      // first point wasn't good.
      PntOut.emplace_back(Ans2);
      return 1;
    } else if (Ans1->distance(Ans2) < Tolerance) {
      // If points too close return only 1 item.
      return 1;
    }
    PntOut.emplace_back(Ans2);
    return 2;
  }
  return 0; // both points imaginary
}

int Line::intersect(PType &VecOut, const Quadratic &Sur) const
/**
For the line that intersects the surfaces
add the point(s) to the VecOut, return number of points
added. It does not check the points for validity.
@param VecOut :: intersection points of the line and surface
@param Sur :: Surface to intersect with a line
@return Number of points found.
*/
{
  const std::vector<double> &BN = Sur.copyBaseEqn();
  const double a(m_origin[0]), b(m_origin[1]), c(m_origin[2]);
  const double d(m_direction[0]), e(m_direction[1]), f(m_direction[2]);
  double Coef[3];
  Coef[0] = BN[0] * d * d + BN[1] * e * e + BN[2] * f * f + BN[3] * d * e + BN[4] * d * f + BN[5] * e * f;
  Coef[1] = 2 * BN[0] * a * d + 2 * BN[1] * b * e + 2 * BN[2] * c * f + BN[3] * (a * e + b * d) +
            BN[4] * (a * f + c * d) + BN[5] * (b * f + c * e) + BN[6] * d + BN[7] * e + BN[8] * f;
  Coef[2] = BN[0] * a * a + BN[1] * b * b + BN[2] * c * c + BN[3] * a * b + BN[4] * a * c + BN[5] * b * c + BN[6] * a +
            BN[7] * b + BN[8] * c + BN[9];

  std::pair<std::complex<double>, std::complex<double>> SQ;
  const int ix = solveQuadratic(Coef, SQ);
  return lambdaPair(ix, SQ, VecOut);
}

int Line::intersect(PType &PntOut, const Plane &Pln) const
/**
For the line that intersects the cylinder generate
add the point to the VecOut, return number of points
added. It does not check the points for validity.

@param PntOut :: Vector of points found by the line/cylinder intersection
@param Pln :: Plane for intersect
@return Number of points found by intersection
*/
{

  const double OdotN = m_origin.scalar_prod(Pln.getNormal());
  const double DdotN = m_direction.scalar_prod(Pln.getNormal());
  if (fabs(DdotN) < Tolerance) // Plane and line parallel
    return 0;
  const double u = (Pln.getDistance() - OdotN) / DdotN;
  if (u <= 0)
    return 0;
  PntOut.emplace_back(getPoint(u));
  return 1;
}

int Line::intersect(PType &PntOut, const Cylinder &cylinder) const
/**
For the line that intersects the cylinder generate
add the point to the VecOut, return number of points
added. It does not check the points for validity.

@param PntOut :: Vector of points found by the line/cylinder intersection
@param cylinder :: Cylinder to intersect line with
@return Number of points found by intersection
*/
{
  const Kernel::V3D center = m_origin - cylinder.getCentre();
  const Kernel::V3D cylinder_axis = cylinder.getNormal();
  const double radius = cylinder.getRadius();
  const double vDn = cylinder_axis.scalar_prod(m_direction);
  const double vDA = cylinder_axis.scalar_prod(center);

  // this is param[0] * x^2 + param[1] * x + param[0]
  // NOTE: Check the documentation page (concept::GeometryofShape) to learn the
  //       detailed derivation of the following formula.
  double quadratic_params[3];
  quadratic_params[0] = 1.0 - (vDn * vDn);
  quadratic_params[1] = 2.0 * (center.scalar_prod(m_direction) - vDA * vDn);
  // norm is the distance from the origin to the center of the cylinder
  quadratic_params[2] = center.norm2() - (radius * radius + vDA * vDA);

  // output parameter
  std::pair<std::complex<double>, std::complex<double>> roots;
  // solve the equation of intersection
  const int num_solutions = solveQuadratic(quadratic_params, roots);
  // This takes the centre displacement into account:
  return lambdaPair(num_solutions, roots, PntOut);
}

int Line::intersect(PType &PntOut, const Sphere &Sph) const
/**
For the line that intersects the cylinder generate
add the point to the VecOut, return number of points
added. It does not check the points for validity.

@param PntOut :: Vector of points found by the line/sphere intersection
@param Sph :: Sphere to intersect line with
@return Number of points found by intersection
*/
{
  // Nasty stripping of useful stuff from sphere
  const Kernel::V3D Ax = m_origin - Sph.getCentre();
  const double R = Sph.getRadius();
  // First solve the equation of intersection
  double C[3];
  C[0] = 1;
  C[1] = 2.0 * Ax.scalar_prod(m_direction);
  C[2] = Ax.scalar_prod(Ax) - R * R;
  std::pair<std::complex<double>, std::complex<double>> SQ;
  const int ix = solveQuadratic(C, SQ);
  return lambdaPair(ix, SQ, PntOut);
}

// SETING

int Line::setLine(const Kernel::V3D &O, const Kernel::V3D &D)
/**
sets the line given the Origne and direction
@param O :: origin
@param D :: direction
@retval  0 ::  Direction == 0 ie no line
@retval 1 :: on success
*/
{
  if (D.nullVector())
    return 0;
  m_origin = O;
  m_direction = normalize(D);
  return 1;
}

void Line::print() const
/**
Print statement for debugging
*/
{
  logger.debug() << "Line == " << m_origin << " :: " << m_direction << '\n';
}

} // namespace Mantid::Geometry
