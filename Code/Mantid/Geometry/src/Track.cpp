#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

#include "Logger.h"
#include "Exception.h"
#include "AuxException.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Track.h"


const double surfaceTolerance(1e-5);       ///< Below this two point touch.

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Track::PLog(Kernel::Logger::get("Track"));

TUnit::TUnit(const Geometry::Vec3D& A,const Geometry::Vec3D& B,
	     const double D,const int ID) :
  PtA(A),PtB(B),Dist(D),Length(A.Distance(B)),ObjID(ID)
  /*!
    Constuctor
    \param A :: Vec3D point to start
    \param B :: Vec3D point to end track
    \param D :: Total distance from start of track
    \param ID :: ID number
   */
{}

TPartial::TPartial(const int ID,const int flag,
		   const Geometry::Vec3D& PVec,const double D) :
  ObjID(ID),Direction(flag),PtA(PVec),Dist(D)
  /*!
    Constuctor
    \param B :: Vec3D point to end track
    \param ID :: ID number
    \param Pt :: Point of intersection
    \param D :: Total distance from start of track
   */
{}

int
TPartial::operator<(const TPartial& A) const
  /*!
    This calculates < based on the length AND
    the flag state. Such that is the length are
    closer than +/- Tolerance then the flag state
    take precidence. 
    \param A :: TPartial object to compare
    \return A is < this
  */
{
  const double dL=fabs(Dist-A.Dist);
  return (dL>surfaceTolerance) ? 
    Dist<A.Dist : Direction<A.Direction;
}

//---------------------------------------------
//           TRACK
// --------------------------------------------

Track::Track(const Geometry::Vec3D& StartPt,
	     const Geometry::Vec3D& UV,const int initObj) : 
  iPt(StartPt),uVec(UV),iObj(initObj)
  /*!
    Constructor
    \param StartPt :: Initial Point
    \param UV :: unit vector of direction
    \param initObj :: inital object identifier
  */ 
{}

Track::Track(const Track& A) :
  iPt(A.iPt),uVec(A.uVec),iObj(A.iObj),
  Link(A.Link),surfPoints(A.surfPoints)
  /*!
    Copy Constructor
    \param A :: Track to copy
  */ 
{}

Track&
Track::operator=(const Track& A)
  /*!
    Assignment operator
    \param A :: Track to copy
    \return *this
  */ 
{
  if (this != &A)
    {
      iPt=A.iPt;
      uVec=A.uVec;
      iObj=A.iObj;
      Link=A.Link;
      surfPoints=A.surfPoints;
    }
  return *this;
}

Track::~Track()
  /*!
    Destructor
  */
{}

void 
Track::setFirst(const Geometry::Vec3D& StartPoint,
		const Geometry::Vec3D& UV)
  /*!
    Sets the first Point
    \param StartPoint :: First Point
    \param UV :: Unit vector
  */
{
  iPt=StartPoint;
  uVec=UV;
  return;
}

int
Track::nonComplete() const
  /*!
    Determines if the track is complete
    \retval 0 :: Complete from Init to end without gaps
    \retval +ve :: Index number of incomplete segment +1
   */
{
  const double TrackTolerance(1e-6);
  if (Link.size()<2)
    return 0;

  LType::const_iterator ac=Link.begin();
  if (iPt.Distance(ac->PtA)>TrackTolerance)
    return 1;
  LType::const_iterator bc=ac;
  bc++;

  while(bc!=Link.end())
    {
      if ((ac->PtB).Distance(bc->PtA)>TrackTolerance)
	return distance(Link.begin(),bc)+1;
      ac++;
      bc++;
    }
  // success
  return 0;
}

void
Track::removeCoJoins()
  /*!
    Remove touching TUnits that have identical
    components
   */
{
  // No work to do:
  if (Link.empty())
    return; 
  // ac == previous : bc = next node.
  LType::iterator ac=Link.begin();
  LType::iterator bc=Link.begin();
  bc++;
  while(bc!=Link.end())
    {
      if (ac->ObjID==bc->ObjID)
        {
	  ac->PtB=bc->PtB;
	  ac->Dist=(ac->PtA).Distance(ac->PtB);
	  ac->Length=bc->Length;
	  Link.erase(bc);
	  bc=ac;
	  bc++;
	}
      else
        {
	  ac++;
	  bc++;
	}
    }
  return;
}

void
Track::addPoint(const int ID,const int Direct,
		const Geometry::Vec3D& Pt) 
  /*!
    Objective is to merge in partial information
    about the beginning and end of the tracks.
    We do not need to keep surfPoints in order
    because that will be done when they are converted into
    TUnits.  

    \param ID :: Id number of the object
    \param Direct :: direction of travel
    \param Pt :: Point to go
   */
{
  surfPoints.push_back(TPartial(ID,Direct,Pt,Pt.Distance(iPt)));
  return;
}

int
Track::addTUnit(const int ID,const Geometry::Vec3D& Apt,
		const Geometry::Vec3D& Bpt)
  /*!
    This adds a whole segment to the track
    \param ID :: Id number of the object
    \param Apt :: first Point
    \param Bpt :: second Point
    \retval Index point 
   */
{
  const double D=Bpt.Distance(Apt);
  // Process First Point
  if (Link.empty())
    {
      Link.push_back(TUnit(Apt,Bpt,D,ID));
      return 0;
    }

//  std::vector<TUnit>::iterator xV=
//	  lower_bound(Link.begin(),Link.end(),D,std::less<TUnit>());

 // Link.insert(xV,TUnit(Apt,Bpt,D,ID));
 // return distance(Link.begin(),xV);
   throw Kernel::Exception::NotImplementedError("Track::addUnit");
   return 0;
}

//void
//Track::buildLink()
//{
//  if (surfPoints.empty())
//    return;
//
//  // First forst surfPoints
//  sort(surfPoints.begin(),surfPoints.end());
//  
//  PType::const_iterator ac=surfPoints.begin();
//  PType::const_iterator bc=ac;
//  bc++;
//  Geometry::Vec3D workPt=iPt;            // last good point
//  // First point is not necessarily in an object
//  // Process first point:
//  if (ac->Direction==1)
//    {
//      addTUnit(0,iPt,ac->PtA);  // from the void
//      workPt=ac->PtA;
//      ac++;
//      bc++;
//    }
//
//  while(ac!=surfPoints.end() || bc!=surfPoints.end())
//    {
//      if (ac->Direction==-1 && bc->Direction==1)
//        {
//	  // Touching point
//	  if (fabs(ac->Dist-bc->Dist)>surfaceTolerance)
//	    {
//	      // track leave ac into bc.
//	      addTUnit(ac->ObjID,workPt,bc->PtA);
//	    }
//	  // Points with intermediate void
//	  else
//	    {
//	      addTUnit(ac->ObjID,workPt,ac->PtA);
//	      addTUnit(0,ac->PtA,bc->PtA);
//	    }
//	  workPt=bc->PtA;
//	  ac++;
//	  ac++;
//	  bc++;  // can I do this past the end ? 
//	  bc++;
//	}
//      else
//        {
//	  std::cerr<<"Error with track points "<<std::endl;
//	}
//    }	
//  surfPoints.clear();        // While vector 
//  return;
//}


} // NAMESPACE Geometry

}  // NAMESPACE Mantid
