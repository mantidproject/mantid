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

#include "Exception.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "GTKreport.h"
#include "FileReport.h"
#include "OutputLog.h"
#include "mathSupport.h"
#include "support.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "BaseVisit.h"
#include "Surface.h"
#include "Plane.h"
#include "Cylinder.h"
#include "Cone.h"
#include "Sphere.h"
#include "General.h"
#include "Line.h"
#include "LineIntersectVisit.h"

namespace Geometry
{

LineIntersectVisit::LineIntersectVisit
  (const Geometry::Vec3D& Pt,const Geometry::Vec3D& uVec) :
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
	    boost::bind(&Geometry::Vec3D::Distance,ATrack.getOrigin(),_1));
  return;
}

}  // NAMESPACE MonteCarlo

