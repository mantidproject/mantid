#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/Matrix.h"
#include <cfloat>
#include <iostream>

namespace Mantid {

namespace Geometry {

using Kernel::Tolerance;
using Kernel::V3D;

// The number of slices to use to approximate a cylinder
int Cylinder::g_nslices = 10;

// The number of slices to use to approximate a cylinder
int Cylinder::g_nstacks = 1;

Cylinder::Cylinder()
    : Quadratic(), Centre(), Normal(1, 0, 0), Nvec(0), Radius(0.0)
/**
 Standard Constructor creats a cylinder (radius 0)
 along the x axis
 */
{
  // Called after it has been sized by Quadratic
  Cylinder::setBaseEqn();
}

Cylinder::Cylinder(const Cylinder &A)
    : Quadratic(A), Centre(A.Centre), Normal(A.Normal), Nvec(A.Nvec),
      Radius(A.Radius)
/**
 Standard Copy Constructor
 @param A :: Cyclinder to copy
 */
{}

Cylinder *Cylinder::clone() const
/**
 Makes a clone (implicit virtual copy constructor)
 @return Copy(*this)
 */
{
  return new Cylinder(*this);
}

Cylinder &Cylinder::operator=(const Cylinder &A)
/**
 Standard Assignment operator
 @param A :: Cylinder object to copy
 @return *this
 */
{
  if (this != &A) {
    Quadratic::operator=(A);
    Centre = A.Centre;
    Normal = A.Normal;
    Nvec = A.Nvec;
    Radius = A.Radius;
  }
  return *this;
}

Cylinder::~Cylinder()
/**
 Standard Destructor
 */
{}

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
  if (!Mantid::Kernel::Strings::section(Line, item) ||
      tolower(item[0]) != 'c' || item.length() < 2 || item.length() > 3)
    return errDesc;

  // Cylinders on X/Y/Z axis
  const int itemPt((item[1] == '/' && item.length() == 3) ? 2 : 1);
  const int ptype = static_cast<int>(tolower(item[itemPt]) - 'x');
  if (ptype < 0 || ptype >= 3)
    return errAxis;
  std::vector<double> norm(3, 0.0);
  std::vector<double> cent(3, 0.0);
  norm[ptype] = 1.0;

  if (itemPt != 1) {
    // get the other two coordinates
    int index((!ptype) ? 1 : 0);
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

  Centre = Kernel::V3D(cent[0], cent[1], cent[2]);
  Normal = Kernel::V3D(norm[0], norm[1], norm[2]);
  Nvec = ptype + 1;
  Radius = R;
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
  if (Nvec) // Nvec =1-3 (point to exclude == Nvec-1)
  {
    if (Radius > 0.0) {
      double x = Pt[Nvec % 3] - Centre[Nvec % 3];
      x *= x;
      double y = Pt[(Nvec + 1) % 3] - Centre[(Nvec + 1) % 3];
      ;
      y *= y;
      const double displace = x + y - Radius * Radius;
      if (fabs(displace / Radius) < Tolerance)
        return 0;
      return (displace > 0.0) ? 1 : -1;
    } else {
      return -1;
    }
  }
  return Quadratic::side(Pt);
}

int Cylinder::onSurface(const Kernel::V3D &Pt) const
/**
 Calculate if the point PT on the cylinder
 @param Pt :: Kernel::V3D to test
 @retval 1 :: on the surface
 @retval 0 :: not on the surface
 */
{
  if (Nvec) // Nvec =1-3 (point to exclude == Nvec-1)
  {
    double x = Pt[Nvec % 3] - Centre[Nvec % 3];
    x *= x;
    double y = Pt[(Nvec + 1) % 3] - Centre[(Nvec + 1) % 3];
    ;
    y *= y;
    return (fabs((x + y) - Radius * Radius) > Tolerance) ? 0 : 1;
  }
  return Quadratic::onSurface(Pt);
}

void Cylinder::setNvec()
/**
 Find if the normal vector allows it to be a special
 type of cylinder on the x,y or z axis
 @return 1,2,3 :: corresponding to a x,y,z alignment
 */
{
  Nvec = 0;
  for (int i = 0; i < 3; i++) {
    if (fabs(Normal[i]) > (1.0 - Tolerance)) {
      Nvec = i + 1;
      return;
    }
  }
  return;
}

void Cylinder::rotate(const Kernel::Matrix<double> &MA)
/**
 Apply a rotation to the cylinder and re-check the
 status of the main axis.
 @param MA :: Rotation Matrix (not inverted)
 */
{
  Centre.rotate(MA);
  Normal.rotate(MA);
  Normal.normalize();
  setNvec();
  Quadratic::rotate(MA);
  return;
}

void Cylinder::displace(const Kernel::V3D &Pt)
/**
 Apply a displacement Pt
 @param Pt :: Displacement to add to the centre
 */
{
  if (Nvec) {
    Centre[Nvec % 3] += Pt[Nvec % 3];
    Centre[(Nvec + 1) % 3] += Pt[(Nvec + 1) % 3];
  } else
    Centre += Pt;
  Quadratic::displace(Pt);
  return;
}

void Cylinder::setCentre(const Kernel::V3D &A)
/**
 Sets the centre Kernel::V3D
 @param A :: centre point
 */
{
  Centre = A;
  setBaseEqn();
  return;
}

void Cylinder::setNorm(const Kernel::V3D &A)
/**
 Sets the centre line unit vector
 A does not need to be a unit vector
 @param A :: Vector along the centre line
 */
{
  Normal = A;
  Normal.normalize();
  setBaseEqn();
  setNvec();
  return;
}

void Cylinder::setBaseEqn()
/**
 Sets an equation of type (cylinder)
 \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
 */
{
  const double CdotN(Centre.scalar_prod(Normal));
  BaseEqn[0] = 1.0 - Normal[0] * Normal[0];           // A x^2
  BaseEqn[1] = 1.0 - Normal[1] * Normal[1];           // B y^2
  BaseEqn[2] = 1.0 - Normal[2] * Normal[2];           // C z^2
  BaseEqn[3] = -2 * Normal[0] * Normal[1];            // D xy
  BaseEqn[4] = -2 * Normal[0] * Normal[2];            // E xz
  BaseEqn[5] = -2 * Normal[1] * Normal[2];            // F yz
  BaseEqn[6] = 2.0 * (Normal[0] * CdotN - Centre[0]); // G x
  BaseEqn[7] = 2.0 * (Normal[1] * CdotN - Centre[1]); // H y
  BaseEqn[8] = 2.0 * (Normal[2] * CdotN - Centre[2]); // J z
  BaseEqn[9] =
      Centre.scalar_prod(Centre) - CdotN * CdotN - Radius * Radius; // K const
  return;
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
  const Kernel::V3D Amov = A - Centre;
  double lambda = Amov.scalar_prod(Normal);
  const Kernel::V3D Ccut = Normal * lambda;
  // The distance is from the centre line to the
  return fabs(Ccut.distance(Amov) - Radius);
}

void Cylinder::write(std::ostream &OX) const
/**
 Write out the cylinder for MCNPX
 @param OX :: output stream
 */
{
  //               -3 -2 -1 0 1 2 3
  const char Tailends[] = "zyx xyz";
  const int Ndir = Normal.masterDir(Tolerance);
  if (Ndir == 0) {
    // general surface
    Quadratic::write(OX);
    return;
  }

  const int Cdir = Centre.masterDir(Tolerance);
  std::ostringstream cx;

  writeHeader(cx);
  cx.precision(Surface::Nprecision);
  // Name and transform

  if (Cdir * Cdir == Ndir * Ndir || Centre.nullVector(Tolerance)) {
    cx << "c";
    cx << Tailends[Ndir + 3] << " "; // set x,y,z based on Ndir
    cx << Radius;
  } else {
    cx << " c/";
    cx << Tailends[Ndir + 3] << " "; // set x,y,z based on Ndir

    if (Ndir == 1 || Ndir == -1)
      cx << Centre[1] << " " << Centre[2] << " ";
    else if (Ndir == 2 || Ndir == -2)
      cx << Centre[0] << " " << Centre[2] << " ";
    else
      cx << Centre[0] << " " << Centre[1] << " ";

    cx << Radius;
  }

  Mantid::Kernel::Strings::writeMCNPX(cx.str(), OX);
  return;
}

double Cylinder::lineIntersect(const Kernel::V3D &Pt,
                               const Kernel::V3D &uVec) const
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
  std::cout << "Axis ==" << Normal << " ";
  std::cout << "Centre == " << Centre << " ";
  std::cout << "Radius == " << Radius << std::endl;
  return;
}

void Cylinder::getBoundingBox(double &xmax, double &ymax, double &zmax,
                              double &xmin, double &ymin, double &zmin) {
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
  if (Normal[0] != 0) {
    xminPoint = Centre + Normal * ((xmin - Centre[0]) / Normal[0]);
    xmaxPoint = Centre + Normal * ((xmax - Centre[0]) / Normal[0]);
    listOfPoints.push_back(xminPoint);
    listOfPoints.push_back(xmaxPoint);
  }

  if (Normal[1] != 0) {
    // ymin plane
    yminPoint = Centre + Normal * ((ymin - Centre[1]) / Normal[1]);
    // ymax plane
    ymaxPoint = Centre + Normal * ((ymax - Centre[1]) / Normal[1]);
    listOfPoints.push_back(yminPoint);
    listOfPoints.push_back(ymaxPoint);
  }
  if (Normal[2] != 0) {
    // zmin plane
    zminPoint = Centre + Normal * ((zmin - Centre[2]) / Normal[2]);
    // zmax plane
    zmaxPoint = Centre + Normal * ((zmax - Centre[2]) / Normal[2]);
    listOfPoints.push_back(zminPoint);
    listOfPoints.push_back(zmaxPoint);
  }
  if (!listOfPoints.empty()) {
    xmin = ymin = zmin = DBL_MAX;
    xmax = ymax = zmax = DBL_MIN;
    for (std::vector<V3D>::const_iterator it = listOfPoints.begin();
         it != listOfPoints.end(); ++it) {
      //			std::cout<<(*it)<<std::endl;
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
    xmax += Radius;
    ymax += Radius;
    zmax += Radius;
    xmin -= Radius;
    ymin -= Radius;
    zmin -= Radius;
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

} // NAMESPACE MonteCarlo

} // NAMESPACE Mantid
