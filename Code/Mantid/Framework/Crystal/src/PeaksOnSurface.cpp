/*WIKI*
Determine whether a peak intersects a surface. Similar to [[PeaksInRegion]]. The vertexes of the surface must be provided. The vertexes must be provided in clockwise ordering starting at the lower left.
*WIKI*/

#include "MantidCrystal/PeaksOnSurface.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include <boost/assign.hpp>
#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
typedef std::vector<double> VecDouble;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PeaksOnSurface)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PeaksOnSurface::PeaksOnSurface() : m_extents(6)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PeaksOnSurface::~PeaksOnSurface()
  {
  }

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string PeaksOnSurface::name() const { return "PeaksOnSurface";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int PeaksOnSurface::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string PeaksOnSurface::category() const { return "crystal";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PeaksOnSurface::initDocs()
  {
    this->setWikiSummary("Find peaks intersecting a single surface region.");
    this->setOptionalMessage(this->getWikiSummary());
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void PeaksOnSurface::init()
  {
    declareProperty(new PropertyWithValue<bool>("CheckPeakExtents", false), "Include any peak in the region that has a shape extent extending into that region.");

    this->initBaseProperties();

    auto manditoryExtents = boost::make_shared<Mantid::Kernel::MandatoryValidator<std::vector<double> > >();

    std::vector<double> vertexDefault;

    declareProperty(new ArrayProperty<double>("Vertex1", vertexDefault, manditoryExtents->clone()),
      "A comma separated list of cartesian coordinates for the lower left vertex of the surface. Values to be specified in the CoordinateFrame choosen.");

    declareProperty(new ArrayProperty<double>("Vertex2", vertexDefault, manditoryExtents->clone()),
      "A comma separated list of cartesian coordinates for the upper left vertex of the surface. Values to be specified in the CoordinateFrame choosen.");

    declareProperty(new ArrayProperty<double>("Vertex3", vertexDefault, manditoryExtents->clone()),
      "A comma separated list of cartesian coordinates for the upper right vertex of the surface. Values to be specified in the CoordinateFrame choosen.");

    declareProperty(new ArrayProperty<double>("Vertex4", vertexDefault, manditoryExtents->clone()),
      "A comma separated list of cartesian coordinates for the lower right vertex of the surface. Values to be specified in the CoordinateFrame choosen.");

  }

  void PeaksOnSurface::validateExtentsInput() const
  {
    /* Parallelepipid volume should be zero if all points are coplanar.
    V = |a.(b x c)|
    */

    V3D a = m_vertex1 - m_vertex2;
    V3D b = m_vertex1 - m_vertex3;
    V3D c = m_vertex1 - m_vertex4;

    if(a.scalar_prod( b.cross_prod(c) ) != 0)
    {
      throw std::invalid_argument("Input vertexes are not coplanar.");
    }

    V3D d = m_vertex2 - m_vertex3; 

    double angle1 = a.angle(b);
    double angle2 = d.angle(b);
    if(angle1 != angle2)
    {
      throw std::invalid_argument("Defined surface is not square sided.");
    }

  }

  bool PeaksOnSurface::pointOutsideAnyExtents(const V3D& testPoint) const
  {
    return true; 
  }


  V3D calculateClosestPoint(const V3D& line, const V3D& lineStart, const V3D& peakCenter)
  {
    V3D ptv = peakCenter - lineStart;
    V3D unitLine = line;
    unitLine.normalize();
    double proj = ptv.scalar_prod(unitLine);
    V3D closestPointOnSegment;
    if(proj <= 0)
    {
      closestPointOnSegment = lineStart; // Start of line
    }
    else if(proj >= line.norm() )
    {
      closestPointOnSegment = lineStart + line; // End of line.
    }
    else
    {
      V3D projectionVector = unitLine * proj;
      closestPointOnSegment = projectionVector + lineStart;
    }
    return closestPointOnSegment;
  }

  bool lineIntersectsSphere(const V3D& line, const V3D& lineStart, const V3D& peakCenter, const double peakRadius)
  {
    
    V3D closestPoint = calculateClosestPoint(line, lineStart, peakCenter);
    V3D distanceV = peakCenter - closestPoint;
    double distance = distanceV.norm();
    return distance <= peakRadius;


    //const double a = line.scalar_prod(line);
    //const double b = 2 * ( (line.X() * ( lineStart.X() - peakCenter.X())) + (line.Y() * ( lineStart.Y() - peakCenter.Y())) + (line.Z() * ( lineStart.Z() - peakCenter.Z())) );
    //const double c = peakCenter.scalar_prod(peakCenter) + lineStart.scalar_prod(lineStart) - (2 * peakCenter.scalar_prod(lineStart)) - peakRadiusSQ; 

    ////const double c = peakCenter.X()*peakCenter.X() + peakCenter.Y()*peakCenter.Y() + peakCenter.Z()*peakCenter.Z() + lineStart.X()*lineStart.X() + lineStart.Y()*lineStart.Y() + lineStart.Z()*lineStart.Z() - 2*(peakCenter.X() * lineStart.X()  + peakCenter.Y() * lineStart.Y() + peakCenter.Z() * lineStart.Z()) - peakRadiusSQ;

    ////completing the square.
    //const double f = (b*b - 4 * a * c);
    //// Solutions to quadratic.
    //const double u1 = (-b + f) / (2 * a);
    //const double u2 = (-b - f) / (2 * a);

    //// Test for line segment passes through sphere in two points.
    //if (((u2 > 0) && (u1 < 1)) || ((u1 > 0) && (u2 < 1)))
    //{
    //  return true;
    //}
    //return false;
  }

  bool PeaksOnSurface::pointInsideAllExtents(const V3D& testPoint, const V3D& peakCenter) const
  {
    const double peakRadius = getPeakRadius();

    /*
    Either, the sphere interesects one of the line segments, which define the bounding edges of the surface,
    OR, the test point lies somewhere on the surface within the extents. We need to check for both.

    The sphere may not necessarily interesect one of the line segments in order to be in contact with the surface, for example, if the peak center as perpendicular to the 
    surface, and the radius such that it only just touched the surface. In this case, no line segments would intersect the sphere.
    */

    return lineIntersectsSphere(m_line1, m_vertex1, peakCenter, peakRadius)
      || lineIntersectsSphere(m_line2, m_vertex2, peakCenter, peakRadius)
      || lineIntersectsSphere(m_line3, m_vertex3, peakCenter, peakRadius)
      || lineIntersectsSphere(m_line4, m_vertex4, peakCenter, peakRadius)
      || (testPoint[0] >= m_extents[0] && testPoint[0] <= m_extents[1]
      && testPoint[1] >= m_extents[2] && testPoint[1] <= m_extents[3] 
      && testPoint[2]>= m_extents[4] && testPoint[2] <= m_extents[5]);
      
  }

  void PeaksOnSurface::checkTouchPoint(const V3D& touchPoint,const  V3D& normal,const  V3D& faceVertex) const
  {
     if( normal.scalar_prod(touchPoint - faceVertex) != 0)
     {
       throw std::runtime_error("Debugging. Calculation is wrong. touch point should always be on the plane!"); // Remove this line later. Check that geometry is setup properly.
     }
  }

  /**
  Implementation of pure virtual method on PeaksIntersection.
  @return Number of faces that the surface has.
  */
  int PeaksOnSurface::numberOfFaces() const
  {
    return 1;
  }

  /**
  Create the faces associated with this shape.
  @return newly created faces
  */
  VecVecV3D PeaksOnSurface::createFaces() const
  {

    //// Clockwise ordering of points around the extents box
    ///*

    //Face is constructed as follows.

    //p2|---|p3
    //  |   |
    //p1|---|p4
    //*

    using boost::assign::list_of;
    const int numberOfFaces = this->numberOfFaces();
    VecVecV3D faces(numberOfFaces);
    faces[0] = list_of(m_vertex1)(m_vertex2)(m_vertex3); // These define a face normal to x at xmin.
    return faces;
  }

  V3D makeV3DFromVector(const VecDouble& vec)
  {
    if(vec.size() != 3)
    {
      throw std::invalid_argument("All Vertex parameter arguments must have 3 entries.");
    }
    return V3D(vec[0], vec[1], vec[2]);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void PeaksOnSurface::exec()
  {
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
    using std::min;
    using std::max;
    m_extents[0] = min(m_vertex1.X(), min(m_vertex2.X(), min(m_vertex3.X(), m_vertex4.X())));
    m_extents[1] = max(m_vertex1.X(), max(m_vertex2.X(), max(m_vertex3.X(), m_vertex4.X())));
    m_extents[2] = min(m_vertex1.Y(), min(m_vertex2.Y(), min(m_vertex3.Y(), m_vertex4.Y())));
    m_extents[3] = max(m_vertex1.Y(), max(m_vertex2.Y(), max(m_vertex3.Y(), m_vertex4.Y())));
    m_extents[4] = min(m_vertex1.Z(), min(m_vertex2.Z(), min(m_vertex3.Z(), m_vertex4.Z())));
    m_extents[5] = max(m_vertex1.Z(), max(m_vertex2.Z(), max(m_vertex3.Z(), m_vertex4.Z())));

    executePeaksIntersection();
  }




} // namespace Crystal
} // namespace Mantid