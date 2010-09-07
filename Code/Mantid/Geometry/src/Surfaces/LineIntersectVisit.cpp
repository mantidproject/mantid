#include "MantidGeometry/Surfaces/LineIntersectVisit.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Support.h"

#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/General.h"
#include <algorithm>
#include <boost/bind.hpp>

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& LineIntersectVisit::PLog(Kernel::Logger::get("LineIntersectVisit"));

LineIntersectVisit::LineIntersectVisit
  (const Geometry::V3D& Pt,const Geometry::V3D& uVec) :
    ATrack(Pt,uVec)
  /*!
    Constructor
  */
{}

void
LineIntersectVisit::Accept(const Surface& Surf)
  /*!
    Process an intersect track
    \param Surf :: Surface to use int line Interesect
  */
{
  throw std::runtime_error("LineIntersectVisit::Accept Surface");
  return;
}

void
LineIntersectVisit::Accept(const Quadratic& Surf)
  /*!
    Process an intersect track
    \param Surf :: Surface to use int line Interesect
  */
{
  ATrack.intersect(PtOut,Surf);
  procTrack();
  return;
}

void
LineIntersectVisit::Accept(const Plane& Surf)
  /*!
    Process an intersect track
    \param Surf :: Surface to use int line Interesect
  */
{
  ATrack.intersect(PtOut,Surf);
  procTrack();
  return;
}

void
LineIntersectVisit::Accept(const Cone& Surf)
  /*!
    Process an intersect track
    \param Surf :: Surface to use int line Interesect
  */
{
  ATrack.intersect(PtOut,Surf);
  procTrack();
  return;
}

void
LineIntersectVisit::Accept(const Cylinder& Surf)
  /*!
    Process an intersect track
    \param Surf :: Surface to use int line Interesect
  */
{
  ATrack.intersect(PtOut,Surf);
  procTrack();
  return;
}

void
LineIntersectVisit::Accept(const Sphere& Surf)
  /*!
    Process an intersect track
    \param Surf :: Surface to use int line Interesect
  */
{
  ATrack.intersect(PtOut,Surf);
  procTrack();
  return;
}

void
LineIntersectVisit::Accept(const General& Surf)
  /*!
    Process an intersect track
    \param Surf :: Surface to use int line Interesect
  */
{
  ATrack.intersect(PtOut,Surf);
  procTrack();
  return;
}

void
LineIntersectVisit::procTrack() 
  /*!
    Sorts the PtOut and distances
    with a closes first order.
  */
{
  // Calculate the distances to the points
  DOut.resize(PtOut.size());
  std::transform(PtOut.begin(),PtOut.end(),DOut.begin(),
	    boost::bind(&Geometry::V3D::distance,ATrack.getOrigin(),_1));
  return;
}

}  // NAMESPACE MonteCarlo


}  // NAMESPACE Mantid
