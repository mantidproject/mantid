#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <list>
#include <stack>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <complex>
#include <boost/regex.hpp>
#include <boost/bind.hpp>

#include "MantidKernel/Logger.h"
#include "AuxException.h"

#include "mathSupport.h"
#include "MantidKernel/Support.h"
#include "MantidGeometry/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/BaseVisit.h"
#include "MantidGeometry/Surface.h"
#include "MantidGeometry/Quadratic.h"
#include "MantidGeometry/Plane.h"
#include "MantidGeometry/Cylinder.h"
#include "MantidGeometry/Cone.h"
#include "MantidGeometry/Sphere.h"
#include "MantidGeometry/General.h"
#include "MantidGeometry/Line.h"
#include "MantidGeometry/LineIntersectVisit.h"

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
LineIntersectVisit::Accept(const Surface&)
  /*!
    Process an intersect track
    \param :: Surface to use int line Interesect
  */
{
  throw ColErr::ExBase(-1,"LineIntersectVisit::Accept Surface");
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
  transform(PtOut.begin(),PtOut.end(),DOut.begin(),
	    boost::bind(&Geometry::V3D::distance,ATrack.getOrigin(),_1));
  return;
}

}  // NAMESPACE MonteCarlo


}  // NAMESPACE Mantid
