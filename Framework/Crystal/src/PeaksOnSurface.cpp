// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/PeaksOnSurface.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::Kernel;
using VecDouble = std::vector<double>;

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PeaksOnSurface)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
PeaksOnSurface::PeaksOnSurface() : m_extents(6) {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string PeaksOnSurface::name() const { return "PeaksOnSurface"; }

/// Algorithm's version for identification. @see Algorithm::version
int PeaksOnSurface::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PeaksOnSurface::category() const { return "Crystal\\Peaks"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PeaksOnSurface::init() {
  this->initBaseProperties();

  auto manditoryExtents = std::make_shared<Mantid::Kernel::MandatoryValidator<std::vector<double>>>();

  declareProperty(std::make_unique<ArrayProperty<double>>("Vertex1", std::vector<double>(), manditoryExtents->clone()),
                  "A comma separated list of cartesian coordinates for the "
                  "lower left vertex of the surface. Values to be specified in "
                  "the CoordinateFrame choosen.");

  declareProperty(std::make_unique<ArrayProperty<double>>("Vertex2", std::vector<double>(), manditoryExtents->clone()),
                  "A comma separated list of cartesian coordinates for the "
                  "upper left vertex of the surface. Values to be specified in "
                  "the CoordinateFrame choosen.");

  declareProperty(std::make_unique<ArrayProperty<double>>("Vertex3", std::vector<double>(), manditoryExtents->clone()),
                  "A comma separated list of cartesian coordinates for the "
                  "upper right vertex of the surface. Values to be specified "
                  "in the CoordinateFrame choosen.");

  declareProperty(
      std::make_unique<ArrayProperty<double>>("Vertex4", std::vector<double>(), std::move(manditoryExtents)),
      "A comma separated list of cartesian coordinates for the "
      "lower right vertex of the surface. Values to be specified "
      "in the CoordinateFrame choosen.");
}

void PeaksOnSurface::validateExtentsInput() const {
  /* Parallelepipid volume should be zero if all points are coplanar.
  V = |a.(b x c)|
  */

  V3D a = m_vertex1 - m_vertex2;
  V3D b = m_vertex1 - m_vertex3;
  V3D c = m_vertex1 - m_vertex4;

  if (a.scalar_prod(b.cross_prod(c)) != 0) {
    throw std::invalid_argument("Input vertexes are not coplanar.");
  }

  V3D d = m_vertex4 - m_vertex2;

  if (b.norm2() != d.norm2()) {
    throw std::invalid_argument("Defined surface is not square sided.");
  }
}

bool PeaksOnSurface::pointOutsideAnyExtents(const V3D & /*testPoint*/) const { return true; }

bool lineIntersectsSphere(const V3D &line, const V3D &lineStart, const V3D &peakCenter, const double peakRadius) {
  V3D peakToStart = peakCenter - lineStart;
  const V3D unitLine = normalize(line);
  double proj = peakToStart.scalar_prod(unitLine); // All we are doing here is
                                                   // projecting the peak to
                                                   // segment start vector onto
                                                   // the segment itself.

  V3D closestPointOnSegment;
  if (proj <= 0) // The projection is outside the segment. So use the start
                 // point of the segment.
  {
    closestPointOnSegment = lineStart; // Start of line
  } else if (proj >= line.norm())      // The projection is greater than the segment
                                       // length. So use the end point of the
                                       // segment.
  {
    closestPointOnSegment = lineStart + line; // End of line.
  } else                                      // The projection falls somewhere between the start and end of the line
                                              // segment.
  {
    V3D projectionVector = unitLine * proj;
    closestPointOnSegment = projectionVector + lineStart;
  }

  return (peakCenter - closestPointOnSegment).norm() <= peakRadius;
}

bool PeaksOnSurface::pointInsideAllExtents(const V3D &testPoint, const V3D &peakCenter) const {
  const double peakRadius = getPeakRadius();

  /*
  Either, the sphere interesects one of the line segments, which define the
  bounding edges of the surface,
  OR, the test point lies somewhere on the surface within the extents. We need
  to check for both.

  The sphere may not necessarily interesect one of the line segments in order to
  be in contact with the surface, for example, if the peak center as
  perpendicular to the
  surface, and the radius such that it only just touched the surface. In this
  case, no line segments would intersect the sphere.
  */

  return lineIntersectsSphere(m_line1, m_vertex1, peakCenter, peakRadius) ||
         lineIntersectsSphere(m_line2, m_vertex2, peakCenter, peakRadius) ||
         lineIntersectsSphere(m_line3, m_vertex3, peakCenter, peakRadius) ||
         lineIntersectsSphere(m_line4, m_vertex4, peakCenter, peakRadius) ||
         (testPoint[0] >= m_extents[0] && testPoint[0] <= m_extents[1] && testPoint[1] >= m_extents[2] &&
          testPoint[1] <= m_extents[3] && testPoint[2] >= m_extents[4] && testPoint[2] <= m_extents[5]);
}

void PeaksOnSurface::checkTouchPoint(const V3D &touchPoint, const V3D &normal, const V3D &faceVertex) const {
  if (normal.scalar_prod(touchPoint - faceVertex) != 0) {
    throw std::runtime_error("Debugging. Calculation is wrong. touch point should always be on the "
                             "plane!"); // Remove this line later. Check that geometry is setup
                                        // properly.
  }
}

/**
Implementation of pure virtual method on PeaksIntersection.
@return Number of faces that the surface has.
*/
int PeaksOnSurface::numberOfFaces() const { return 1; }

/**
Create the faces associated with this shape.
@return newly created faces
*/
VecVecV3D PeaksOnSurface::createFaces() const {

  //// Clockwise ordering of points around the extents box
  ///*

  // Face is constructed as follows.

  // p2|---|p3
  //  |   |
  // p1|---|p4
  //*

  const int facesN = this->numberOfFaces();
  VecVecV3D faces(facesN);
  faces[0] = {m_vertex1, m_vertex2, m_vertex3}; // These define a face normal
                                                // to x at xmin.
  return faces;
}

V3D makeV3DFromVector(const VecDouble &vec) {
  if (vec.size() != 3) {
    throw std::invalid_argument("All Vertex parameter arguments must have 3 entries.");
  }
  return V3D(vec[0], vec[1], vec[2]);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PeaksOnSurface::exec() {
  VecDouble vertex1 = this->getProperty("Vertex1");
  VecDouble vertex2 = this->getProperty("Vertex2");
  VecDouble vertex3 = this->getProperty("Vertex3");
  VecDouble vertex4 = this->getProperty("Vertex4");

  // Check vertexes, make a V3D and assign..
  m_vertex1 = makeV3DFromVector(vertex1);
  m_vertex2 = makeV3DFromVector(vertex2);
  m_vertex3 = makeV3DFromVector(vertex3);
  m_vertex4 = makeV3DFromVector(vertex4);

  // Template method. Validate the extents inputs.
  validateExtentsInput();

  // Create line segments for boundary calculations.
  m_line1 = m_vertex2 - m_vertex1;
  m_line2 = m_vertex3 - m_vertex2;
  m_line3 = m_vertex4 - m_vertex3;
  m_line4 = m_vertex1 - m_vertex4;

  // Determine minimum and maximum in x, y and z.
  using std::max;
  using std::min;
  m_extents[0] = min(m_vertex1.X(), min(m_vertex2.X(), min(m_vertex3.X(), m_vertex4.X())));
  m_extents[1] = max(m_vertex1.X(), max(m_vertex2.X(), max(m_vertex3.X(), m_vertex4.X())));
  m_extents[2] = min(m_vertex1.Y(), min(m_vertex2.Y(), min(m_vertex3.Y(), m_vertex4.Y())));
  m_extents[3] = max(m_vertex1.Y(), max(m_vertex2.Y(), max(m_vertex3.Y(), m_vertex4.Y())));
  m_extents[4] = min(m_vertex1.Z(), min(m_vertex2.Z(), min(m_vertex3.Z(), m_vertex4.Z())));
  m_extents[5] = max(m_vertex1.Z(), max(m_vertex2.Z(), max(m_vertex3.Z(), m_vertex4.Z())));

  executePeaksIntersection();
}

} // namespace Mantid::Crystal
