#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stack>
#include <string>
#include <sstream>
#include <algorithm>
#include <boost/regex.hpp>

#include "Logger.h"
#include "AuxException.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "support.h"
#include "regexSupport.h"
#include "FileReport.h"
#include "GTKreport.h"
#include "OutputLog.h"
#include "BaseVisit.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Track.h"
#include "Surface.h"
#include "Rules.h"
#include "Object.h"
#include "Material.h"
#include "SamGeometry.h"

namespace Mantid
{

namespace Geometry
{

Logger& SamGeometry::PLog = Logger::get("SamGeometry");
SamGeometry::SamGeometry()
  /*!
    Default Constructor
  */
{}
  
SamGeometry::SamGeometry(const SamGeometry& A) :
  Items(A.Items),SNum(A.SNum),
  SurToObj(A.SurToObj),MatMap(A.MatMap)
  /*!
    Copy Constructor
    \param A :: SamGeometry object to copy
  */
{}

SamGeometry&
SamGeometry::operator=(const SamGeometry& A)
  /*!
    Assignment operator
    \param A :: SamGeometry object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      Items=A.Items;
      SNum=A.SNum;
      SurToObj=A.SurToObj;
      MatMap=A.MatMap;
    }
  return *this;
}

SamGeometry::~SamGeometry()
  /*!
    Destructor
  */
{}

/* void
SamGeometry::assignMaterial(const int ObjNum,const int MtNum)
  /*!
    Given Obj and Matrial numbers, link
    the two object together.
    \throw InContainerError<int> when ObjNum or MtNum does not exist
    \param ObjNum :: Object Index
    \param MtNum :: Material Nubmer
   * /
{
  MATMAP::iterator mc=MatMap.find(MtNum);
  if (mc==MatMap.end())
    throw ColErr::InContainerError<int>(MtNum,"SamGeometry::assignMaterial MatNumber");

  
//  if (vc==.end())
//    throw ColErr::IncontainerError<int>(MtNum,"SamGeometry::assignMaterial MatNumber");
  
  
}
*/
void
SamGeometry::addObject(const Object& Obj)
   /*!
     Add an object to the list. Note that
     it changes the copy s object name to the vector place 
     - Customary to set a void cell to be the first input
     but doesn't matter.
     \param Obj :: Object to add
     
   */
{
  Items.push_back(Obj);
  Items.back().setName(Items.size()-1);
  return;
}

void
SamGeometry::setMaterial(const int Index,const Material* MatPtr)
   /*!
     Adds a material to the main list.
     \param Index :: Index of the material to add
     \param MatObj :: Object to add 
   */
{
  if (Index<=0) 
    throw ColErr::IndexError(Index,10000,"SamGeometry::addMaterial");
  MatMap[Index]=MatPtr;
  return;
}


void
SamGeometry::createTable()
  /*!
    Create the cross tables SNum and SurToObj
    using Items.
    - cell numbers run from 1->N because 0 is the
    void cell.
    - 
  */
{
  ISTORE::iterator vc;
  typedef std::vector<const Surface*> SVEC;
  int CellNum(0);
  for(vc=Items.begin();vc!=Items.end();vc++)
    {
      vc->createSurfaceList();                // get surfaces
      SVEC SItem=vc->getSurfacePtr();         
      SVEC::const_iterator vc;
      // for each surface get 
      for(vc=SItem.begin();vc!=SItem.end();vc++)   
        {
	  // create surface number to point index:
	  const int SN=(*vc)->getName();
	  if (SNum.find(SN)!=SNum.end())
	    SNum.insert(std::pair<int,const Surface*>(SN,*vc));
	  // create surface number to object index [multimap]
	  // Care: need find unit surfN->objectNum 
	  //     : find matched first only.
	  MOBJ::iterator vc=SurToObj.find(SN);
	  while(vc!=SurToObj.end() && vc->second!=CellNum
		&& vc->first==SN)
	    vc++;

	  if (vc==SurToObj.end() || vc->first!=SN)
	    SurToObj.insert(MOBJ::value_type(SN,CellNum));
	}
      CellNum++;
    }

  return;
}

const Object&
SamGeometry::getObject(const int Index) const
  /*!
    Accessor to the Objects
    \param Index :: Index item of object to get
  */
{
  if (Index<0 || Index>static_cast<int>(Items.size()))
    throw ColErr::IndexError(Index,Items.size(),"SamGeometry::getObject const");
  return Items[Index];
}

Object&
SamGeometry::getObject(const int Index)
  /*!
    Accessor to the Objects
    \param Index :: Index item of object to get
  */
{
  if (Index<0 || Index>static_cast<int>(Items.size()))
    throw ColErr::IndexError(Index,Items.size(),"SamGeometry::getObject");
  return Items[Index];
}

int
SamGeometry::findCell(const Geometry::Vec3D& Pt) const
  /*!
    This exhausively searches for the point in the objects.
    It relies on the face that SamGeometry::Items have had
    the object names changed to the correct insertion point.
    This is a safe call if Items are reordered.
    \param Pt :: Point to test
    \reval -1 :: Void
    \retval Index+1 of the object in Items
   */
{
  ISTORE::const_iterator vc;
  for(vc=Items.begin();vc!=Items.end();vc++)
    {
      if (vc->isValid(Pt))
	return distance(vc,Items.begin());
    }
  return -1;
}


int
SamGeometry::findCell(const Geometry::Vec3D& Pt,const int testID) const
  /*!
    This exhausively searches for the point in the objects.
    It relies on the face that SamGeometry::Items have had
    the object names changed to the correct insertion point.
    \param Pt :: Point to test
    \param testID :: Possible cell value to start.
    \reval 0 :: Void
    \retval Index+1 of the object in Items
   */
{
  if (testID>=0 && testID<static_cast<int>(Items.size()))
    {
      if (Items[testID].isValid(Pt))
	return testID;
    }
  return findCell(Pt);
}

Geometry::Track
SamGeometry::buildTrack(const Geometry::Vec3D& Pt,
		   const Geometry::Vec3D& uVec) const
  /*!
    Calculate a track through this sample from Pt, uVec
    - create track
    - Calculate if the track intercepts the current cell.
    - If it does then add all connected cells to the search list 
    \param Pt :: Initial Point
    \param uVec :: Direction vector
    \return Track
   */
{
  // Which cell are we in (-1 ==> void)
  // This is an expensive call so use guide number if possible
  const int cellID=findCell(Pt); 
  // Create a track 
  Geometry::Track OutTrack(Pt,uVec,cellID);

  // Process the current cell and all touching cells 
  // and the cells that touch them...
  
  // List of cells already processed
  std::set<int> procCells;    
  std::stack<int> searchStack;
  searchStack.push(cellID);

  // SEARCH SAMPLE
  // Search while still connected cells
  while(!searchStack.empty())
    {
      const int cNum=searchStack.top();
      searchStack.pop();
      procCells.insert(cNum);
      const Object& Oref(Items[cellID]);          
      const int nCnt=Oref.interceptSurface(OutTrack);
      // interception found
      if (nCnt)
        {
	  // Get list of cells with this surface:
	  MOBJ::const_iterator mp=SurToObj.find(cNum);
	  while(mp!=SurToObj.end() && mp->first==cNum)
	    {
	      // has this already been searched ?
	      // or is already scheduled for search
	      std::set<int>::const_iterator sc=
		procCells.find(mp->second);
	      if (sc==procCells.end())        // New cell
	        {
		  searchStack.push(mp->second);
		  procCells.insert(mp->second);
		}
	    }
	}
    }
  // CREATE OUTPUT TRACK
  OutTrack.buildLink();
  return OutTrack;
}

	  
double
SamGeometry::outAtten(const double wavelength,int cellID,
		      const Geometry::Vec3D& Pt,const Geometry::Vec3D& uVec) const
  /*!
    Calculate the attenuation coefficient from the Pt in direction
    uVec. 

    The process is
    - create track
    - Calculate if the track intercepts the current cell.
    - If it does then add all connected cells to the search list 
    \param wavelength :: Wavelength (Angstrom)
    \param cellID :: Object that Pt starts in 
    \param uVec :: Direction vector
    \return attenuation coefficient
   */
{

  // Create a track 
  Geometry::Track OutTrack=buildTrack(Pt,uVec);
  // Process the current cell and all touching cells 
  // and the cells that touch them...
  
  double weight(1.0);            // Weight of 
  Geometry::Track::LType::const_iterator vc;
  for(vc=OutTrack.begin();vc!=OutTrack.end();vc++)
    {
      const Object& objRef=Items[vc->ObjID];
      const int matNum=objRef.getMat();
      // Null does not need to be summed:
      if (matNum)
        {
	  MATMAP::const_iterator mc=MatMap.find(matNum);
	  if (mc==MatMap.end()) 
	    throw ColErr::InContainerError<int>(matNum,"SamGeometry:OutAttn");
	  weight*=mc->second->calcAtten(wavelength,vc->Length);
	} 
    }
  return weight;
}

void
SamGeometry::procXML(XML::XMLcollect& XOut) const
  /*!
    This writes the XML schema. This is not
    a complete write out since createTable is
    called after an import.
    \param XOut :: Output parameter
   */
{
  ISTORE::const_iterator vc;
  for(vc=Items.begin();vc!=Items.end();vc++)
    {
      XOut.addGrp("Object");
      vc->procXML(XOut);
      XOut.closeGrp();
    }
  return;
}


int
SamGeometry::importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
		  const int singleFlag)
  /*!
    Given a distribution that has been put into an XML base set.
    The XMLcollection need to have the XMLgroup pointing to
    the section for this DExpt.

    \param SK :: IndexIterator object
    \param singleFlag :: single pass through to determine if has key
    (only for virtual base object)
   */
{
  int errCnt(0);
  int levelExit(SK.getLevel());
  do
    {
      if (*SK)
        {
	  const std::string& KVal=SK->getKey();
	  const XML::XMLread* RPtr=dynamic_cast<const XML::XMLread*>(*SK);
	  int errNum(1);
	  if (RPtr)
	    {
	      if (KVal=="Object")
	        {
		  Object OInput;
		  errNum=OInput.importXML(SK,singleFlag);
		  if (!errNum)
		    Items.push_back(OInput);
		}		    
	    }
	  if (errNum)
	    {
	      errCnt++;                 // Not good....
	      ELog::EMessages.Estream()
		<<"SamGeometry::importXML :: Failed on key: "<<KVal;
	      ELog::EMessages.report(2);
	    }
	  // Post processing
	  if (!singleFlag) 
	    SK++;
	}
    } while (!singleFlag && SK.getLevel()>=levelExit);

  return errCnt;
}
	  
}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid
