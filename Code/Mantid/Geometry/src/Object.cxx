#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <stack>
#include <string>
#include <sstream>
#include <algorithm>
#include <boost/regex.hpp>

#include "Logger.h"
#include "Exception.h"
#include "FileReport.h"
#include "GTKreport.h"
#include "OutputLog.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "support.h"
#include "regexSupport.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Track.h"
#include "Line.h"
#include "BaseVisit.h"
#include "LineIntersectVisit.h"
#include "Surface.h"
#include "Rules.h"
#include "Object.h"

namespace Mantid
{

namespace Geometry
{

Logger& Object::PLog = Logger::get("Object");
const double OTolerance(1e-6);

Object::Object() :
  ObjName(0),MatN(-1),Tmp(300),density(0.0),TopRule(0)
  /*!
    Defaut constuctor, set temperature to 300K and material to vacuum
  */
{}

Object::Object(const Object& A) :
  ObjName(A.ObjName),MatN(A.MatN),Tmp(A.Tmp),density(A.density),
  TopRule((A.TopRule) ? A.TopRule->clone() : 0),
  SurList(A.SurList)
  /*!
    Copy constructor
    \param A :: Object to copy
  */
{}

Object&
Object::operator=(const Object& A)
  /*!
    Assignment operator 
    \param A :: Object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      ObjName=A.ObjName;
      MatN=A.MatN;
      Tmp=A.Tmp;
      density=A.density;
      delete TopRule;
      TopRule=(A.TopRule) ? A.TopRule->clone() : 0;
      SurList=A.SurList;
    }
  return *this;
}

Object::~Object()
  /*!
    Delete operator : removes Object tree
  */
{
  delete TopRule;
}

int
Object::setObject(const int ON,const std::string& Ln)
 /*! 
   Object line ==  cell 
   \param ON :: Object name
   \param Ln :: Input string must:  {rules}
   \retval 1 on success
   \retval 0 on failure
 */
{
  // Split line
  std::string part;  
  const boost::regex letters("[a-zA-Z]");  // Does the string now contain junk...
  if (StrFunc::StrLook(Ln,letters))
    return 0;
  
  if (procString(Ln))     // this currently does not fail:
    {
      SurList.clear();
      ObjName=ON;
      return 1;
    }

  // failure
  return 0;
}

void
Object::convertComplement(const std::map<int,Object>& MList) 
  /*!
    Returns just the cell string object
    \param MList :: List of indexable Hulls
    \return Cell String (from TopRule)
    \todo Break infinite recusion
  */
{
  this->procString(this->cellStr(MList));
  return;
}


std::string
Object::cellStr(const std::map<int,Object>& MList) const
  /*!
    Returns just the cell string object
    \param MList :: List of indexable Hulls
    \return Cell String (from TopRule)
    \todo Break infinite recusion
  */
{
  std::string TopStr=this->topRule()->display();
  std::string::size_type pos=TopStr.find('#');
  std::ostringstream cx;
  while(pos!=std::string::npos)
    {
      pos++;
      cx<<TopStr.substr(0,pos);            // Everything including the #
      int cN(0);
      const int nLen=StrFunc::convPartNum(TopStr.substr(pos),cN);
      if (nLen>0)
        {
	  cx<<"(";
	  std::map<int,Object>::const_iterator vc=MList.find(cN);
	  if (vc==MList.end())
	    throw ColErr::InContainerError<int>(cN,"Object::cellStr");
	  // Not the recusion :: This will cause no end of problems 
	  // if there is an infinite loop.
	  cx<<vc->second.cellStr(MList);
	  cx<<") ";
	  pos+=nLen;
	}
      TopStr.erase(0,pos);
      pos=TopStr.find('#');
    }
  cx<<TopStr;
  return cx.str();
}

int
Object::complementaryObject(const int Cnum,std::string& Ln)
  /*!
    Calcluate if there are any complementary components in
    the object. That is lines with #(....)
    \throws ColErr::ExBase :: Error with processing
    \param Ln :: Input string must:  ID Mat {Density}  {rules}
    \param Cnum :: Number for cell since we don't have one
    \retval 0 on no work to do
    \retval 1 :: A (maybe there are many) #(...) object found
 */
{
  std::string::size_type posA=Ln.find("#(");
  // No work to do ?
  if (posA==std::string::npos)
    return 0;
  posA+=2;

  // First get the area to be removed
  int brackCnt;
  std::string::size_type posB;
  posB=Ln.find_first_of("()",posA);
  if (posB==std::string::npos)
    throw ColErr::ExBase(posA,"Object::complemenet :: "+Ln);

  brackCnt=(Ln[posB] == '(') ? 1 : 0;
  while (posB!=std::string::npos && brackCnt)
    {
      posB=Ln.find_first_of("()",posB);
      brackCnt+=(Ln[posB] == '(') ? 1 : -1;
      posB++;
    }

  std::string Part=Ln.substr(posA,posB-(posA+1));

  ObjName=Cnum;
  MatN=0;
  density=0;
  if (procString(Part))
    {
      SurList.clear();
      Ln.erase(posA-1,posB+1);  //Delete brackets ( Part ) .
      std::ostringstream CompCell;
      CompCell<<Cnum<<" ";
      Ln.insert(posA-1,CompCell.str());
      return 1;
    }

  throw ColErr::ExBase(0,"Object::complemenet :: "+Part);
  return 0;
}

int
Object::hasComplement() const
  /*!
    Determine if the object has a complementary object

    \retval 1 :: true 
    \retval 0 :: false
  */
{

  if (TopRule)
    return TopRule->isComplementary();
  return 0;
}

int 
Object::populate(const std::map<int,Surface*>& Smap)
  /*! 
     Goes through the cell objects and adds the pointers
     to the SurfPoint keys (using their keyN) 
     \param Smap :: Map of surface Keys and Surface Pointers
     \retval 1000+ keyNumber :: Error with keyNumber
     \retval 0 :: successfully populated all the whole Object.
  */
{
  std::deque<Rule*> Rst;
  Rst.push_back(TopRule);
  Rule* TA, *TB;      //Tmp. for storage

  int Rcount(0);
  while(Rst.size())
    {
      Rule* T1=Rst.front();
      Rst.pop_front();
      if (T1)
        {
	  // if an actual surface process :
	  SurfPoint* KV=dynamic_cast<SurfPoint*>(T1);
	  if (KV)
	    {
	      // Ensure that we have a it in the surface list:
	      std::map<int,Surface*>::const_iterator mf=Smap.find(KV->getKeyN());
	      if (mf!=Smap.end())
	        {
		  KV->setKey(mf->second);
		  Rcount++;
		}
	      else 
	        {
		  ELog::EMessages.report("Error finding key:",1);
		  throw ColErr::InContainerError<int>(KV->getKeyN(),"Object::populate");
		}
	    }
	  // Not a surface : Determine leaves etc and add to stack:
	  else
	    {
	      TA=T1->leaf(0);
	      TB=T1->leaf(1);
	      if (TA)
		Rst.push_back(TA);
	      if (TB)
		Rst.push_back(TB);
	    }
	}
    }
#if debugObject
  std::cerr<<"Populated surfaces =="<<Rcount<<" in "<<ObjName<<std::endl;
#endif
  return 0;
}

int
Object::procPair(std::string& Ln,std::map<int,Rule*>& Rlist,int& compUnit) const
  /*!
    This takes a string Ln, finds the first two 
    Rxxx function, determines their join type 
    make the rule,  adds to vector then removes two old rules from 
    the vector, updates string
    \param Ln :: String to porcess
    \param Rlist :: Map of rules (added to)
    \param CompUnit :: Last computed unit
    \retval 0 :: No rule to find
    \retval 1 :: A rule has been combined
  */
{
  unsigned int Rstart;
  unsigned int Rend;
  int Ra,Rb;

  for(Rstart=0;Rstart<Ln.size() &&
	Ln[Rstart]!='R';Rstart++);

  int type=0;      //intersection 

//plus 1 to skip 'R'
  if (Rstart==Ln.size() || !StrFunc::convert(Ln.c_str()+Rstart+1,Ra) ||
    Rlist.find(Ra)==Rlist.end())  
    return 0;

  for(Rend=Rstart+1;Rend<Ln.size() &&
	Ln[Rend]!='R';Rend++)
    {
      if (Ln[Rend]==':')
	type=1;        //make union
    }
  if (Rend==Ln.size() || !StrFunc::convert(Ln.c_str()+Rend+1,Rb) ||
      Rlist.find(Rb)==Rlist.end())  
    return 0;

  // Get end of number (digital)
  for(Rend++;Rend<Ln.size() && 
	Ln[Rend]>='0' && Ln[Rend]<='9';Rend++);     
  
  // Get rules
  Rule* RRA=Rlist[Ra];
  Rule* RRB=Rlist[Rb];
  Rule* Join=(type) ? static_cast<Rule*>(new Union(RRA,RRB)) :
                      static_cast<Rule*>(new Intersection(RRA,RRB));
  Rlist[Ra]=Join;
  Rlist.erase(Rlist.find(Rb));     

  // Remove space round pair
  int fb;
  for(fb=Rstart-1;fb>=0 && Ln[fb]==' ';fb--);
  Rstart=(fb<0) ? 0 : fb; 
  for(fb=Rend;fb<static_cast<int>(Ln.size()) && Ln[fb]==' ';fb++); 
  Rend=fb;

  std::stringstream cx;
  cx<<" R"<<Ra<<" ";
  Ln.replace(Rstart,Rend,cx.str());
  compUnit=Ra;
  return 1;
}

CompGrp*
Object::procComp(Rule* RItem) const
  /*!
    Taks a Rule item and makes it a complementary group
    \param RItem to encapsulate
  */
{
  if (!RItem)
    return new CompGrp();
  
  Rule* Pptr=RItem->getParent();
  CompGrp* CG=new CompGrp(Pptr,RItem);
  if (Pptr)
    {
      const int Ln=Pptr->findLeaf(RItem);
      Pptr->setLeaf(CG,Ln);
    }
  return CG;
}

int
Object::isOnSide(const Geometry::Vec3D& Pt) const
  /*!
    - (a) Uses the Surface list to check those surface
    that the point is on.
    - (b) Creates a list of normals to the touching surfaces
    - (c) Checks if normals and "normal pair bisection vector" 
        are contary.
          If any are found to be so the the point is 
	  on a surface. 
    - (d) Return 1 / 0 depending on test (c)

    ** I believe that this test is complete, ie it will not
    ever return 0 for a surface no matter how non-convex.
    \param Pt :: Point to check
    \returns 1 if the point is on the surface
  */
{
  std::list<Geometry::Vec3D> Snorms;     // Normals from the constact surface.

  std::vector<const Surface*>::const_iterator vc;
  for(vc=SurList.begin();vc!=SurList.end();vc++)
    {
      if ((*vc)->onSurface(Pt))
	{
	  Snorms.push_back((*vc)->surfaceNormal(Pt));
	  // can check direct normal here since one success
	  // means that we can return 1 and finish
	  if (checkSurfaceValid(Pt,Snorms.back()))
	    return 1;
	}
    }
  std::list<Geometry::Vec3D>::const_iterator xs,ys;
  Geometry::Vec3D NormPair;
  for(xs=Snorms.begin();xs!=Snorms.end();xs++)
    for(ys=xs,ys++;ys!=Snorms.end();ys++)
      {
	NormPair=(*ys)+(*xs);
	NormPair.makeUnit();
	if (checkSurfaceValid(Pt,NormPair))
	  return 1;
      }
  // Ok everthing failed return 0;
  return 0;
}

int
Object::checkSurfaceValid(const Geometry::Vec3D& C,const Geometry::Vec3D& Nm) const
  /*!
    Determine if a point is valid by checking both
    directions of the normal away from the line
    A good point will have one valid and one invalid.
    \param C :: Point on a basic surface to check 
    \param Nm :: Direction +/- to be checked
    \retval +/-1 ::  C+Nm or C-Nm are both the same
    \retval 0 :: otherwize
  */
{
  int status(0);
  Geometry::Vec3D tmp=C+Nm*(OTolerance*5.0);
  status= (!isValid(tmp)) ? 1 : -1;
  tmp-= Nm*(OTolerance*10.0);
  status+= (!isValid(tmp)) ? 1 : -1;
  return status/2;
}

int
Object::isValid(const Geometry::Vec3D& Pt) const
/*! 
  Determines is Pt is within the object 
  or on the surface
  \param Pt :: Point to be tested
  \returns 1 if true and 0 if false
*/
{
  if (!TopRule)
    return 0;
  return TopRule->isValid(Pt);
}

int
Object::isValid(const std::map<int,int>& SMap) const
/*! 
  Determines is group of surface maps are valid
  \param SMap :: map of SurfaceNumber : status
  \returns 1 if true and 0 if false
*/
{
  if (!TopRule)
    return 0;
  return TopRule->isValid(SMap);
}
  
int
Object::createSurfaceList(const int outFlag)
  /*! 
    Uses the topRule* to create a surface list
    by iterating throught the tree
    \return 1 (should be number of surfaces)
  */
{ 
  SurList.clear();
  std::stack<const Rule*> TreeLine;
  TreeLine.push(TopRule);
  while(!TreeLine.empty())
    {
      const Rule* tmpA=TreeLine.top();
      TreeLine.pop();
      const Rule* tmpB=tmpA->leaf(0);
      const Rule* tmpC=tmpA->leaf(1);
      if (tmpB || tmpC)
        {
	  if (tmpB)
	    TreeLine.push(tmpB);
	  if (tmpC)
	    TreeLine.push(tmpC);
	}
      else
        {
	  const SurfPoint* SurX=dynamic_cast<const SurfPoint*>(tmpA);
	  if (SurX)
	    SurList.push_back(SurX->getKey());
//	  else if (dynamic_cast<const CompObj*>(tmpA))
//	    {
//	      std::cerr<<"Ojbect::is Complementary"<<std::endl;
//	    }
//	  else
//	    std::cerr<<"Object::Error with surface List"<<std::endl;
	}
    }
  if (outFlag)
    {

      std::vector<const Surface*>::const_iterator vc;
      for(vc=SurList.begin();vc!=SurList.end();vc++)
        {
	  std::cerr<<"Point == "<<reinterpret_cast<long int>(*vc)<<std::endl;
	  std::cerr<<(*vc)->getName()<<std::endl;
	}
    }
  return 1;
}

std::vector<int>
Object::getSurfaceIndex() const
  /*!
    Returns all of the numbers of surfaces
    \return Surface numbers
  */
{
  std::vector<int> out;
  transform(SurList.begin(),SurList.end(),
	    std::insert_iterator<std::vector<int> >(out,out.begin()),
	    std::mem_fun(&Surface::getName));
  return out;
}

int
Object::removeSurface(const int SurfN)
  /*! 
    Removes a surface and then re-builds the 
    cell. This could be done by just removing 
    the surface from the object.
    \param SurfN :: Number for the surface
    \return number of surfaces removes
  */
{ 
  if (!TopRule)
    return -1;
  const int cnt=Rule::removeItem(TopRule,SurfN);
  if (cnt)
    createSurfaceList();
  return cnt;
}

int
Object::substituteSurf(const int SurfN,const int NsurfN,Surface* SPtr)
  /*! 
    Removes a surface and then re-builds the 
    cell.
    \param SurfN :: Number for the surface
    \param NsurfN :: New surface number
    \param SPtr :: Surface pointer for surface NsurfN
    \return number of surfaces substituted
  */
{ 
  if (!TopRule)
    return 0;
  const int out=TopRule->substituteSurf(SurfN,NsurfN,SPtr);
  if (out)
    createSurfaceList();
  return out;
}

void 
Object::print() const
  /*!
    Prints almost everything 
  */
{
  std::deque<Rule*> Rst;
  std::vector<int> Cells;
  int Rcount(0);
  Rst.push_back(TopRule);
  Rule* TA, *TB;      //Temp. for storage

  while(Rst.size())
    {
      Rule* T1=Rst.front();
      Rst.pop_front();
      if (T1)
        {
	  Rcount++;
	  SurfPoint* KV=dynamic_cast<SurfPoint*>(T1);
	  if (KV)
	    Cells.push_back(KV->getKeyN());
	  else
	    {
	      TA=T1->leaf(0);
	      TB=T1->leaf(1);
	      if (TA)
		Rst.push_back(TA);
	      if (TB)
		Rst.push_back(TB);
	    }
	}
    }

  std::cout<<"Name == "<<ObjName<<std::endl;
  std::cout<<"Material == "<<MatN<<std::endl;
  std::cout<<"Rules == "<<Rcount<<std::endl;
  std::vector<int>::const_iterator  mc;
  std::cout<<"Surface included == ";
  for(mc=Cells.begin();mc<Cells.end();mc++)
    {
      std::cout<<(*mc)<<" ";
    }
  std::cout<<std::endl;
  return;
}


void
Object::makeComplement()
  /*!
    Takes the complement of a group
   */
{
  Rule* NCG=procComp(TopRule);
  TopRule=NCG;
  return;
}


void 
Object::printTree() const
  /*!
    Displays the rule tree 
  */
{
  std::cout<<"Name == "<<ObjName<<std::endl;
  std::cout<<"Material == "<<MatN<<std::endl;
  std::cout<<TopRule->display()<<std::endl;
  return;
}


std::string
Object::cellCompStr() const
  /*!
    Write the object to a string.
    This includes only rules.
    \return Object Line
  */
{
  std::ostringstream cx;
  if (TopRule)
    cx<<TopRule->display();
  return cx.str();
}

std::string
Object::str() const
  /*!
    Write the object to a string.
    This includes the Name , material and density but 
    not post-fix operators
    \return Object Line
  */
{
  std::ostringstream cx;
  if (TopRule)
    {
      cx<<ObjName<<" "<<MatN;
      if (MatN!=0)
	cx<<" "<<density;
      cx<<" "<<TopRule->display();
    }
  return cx.str();
}

void 
Object::write(std::ostream& OX) const
  /*!
    Write the object to a standard stream
    in standard MCNPX output format.
    \param OX :: Output stream (required for multiple std::endl)
  */
{
  std::ostringstream cx;
  cx.precision(10);
  cx<<str();
  if (Tmp!=300.0)
    cx<<" "<<"tmp="<<Tmp*8.6173422e-11;
  StrFunc::writeMCNPX(cx.str(),OX);
  return;
}

int
Object::procString(const std::string& Line) 
  /*!
    Processes the cell string. This is an internal function
    to process a string with 
    - String type has #( and ( )
    \param Ln :: String value
    \returns 1 on success
  */
{
  delete TopRule;
  TopRule=0;
  std::map<int,Rule*> RuleList;    //List for the rules 
  int Ridx=0;                     //Current index (not necessary size of RuleList 
  // SURFACE REPLACEMENT
  // Now replace all free planes/Surfaces with appropiate Rxxx
  SurfPoint* TmpR(0);       //Tempory Rule storage position
  CompObj* TmpO(0);         //Tempory Rule storage position

  std::string Ln=Line;
  // Remove all surfaces :
  std::ostringstream cx; 
  for(int i=0;i<static_cast<int>(Ln.length());i++)
    {
      if (isdigit(Ln[i]) || Ln[i]=='-')
        {
	  int SN;
	  int nLen=StrFunc::convPartNum(Ln.substr(i),SN);
	  if (!nLen)
	    throw ColErr::InvalidLine(Ln,"Object::ProcString",i);
	  // Process #Number
	  if (i!=0 && Ln[i-1]=='#')
	    {
	      TmpO=new CompObj();
	      TmpO->setObjN(SN);
	      RuleList[Ridx]=TmpO;
	    }
	  else       // Normal rule
	    {
	      TmpR=new SurfPoint();
	      TmpR->setKeyN(SN);
	      RuleList[Ridx]=TmpR;
	    }
	  cx<<" R"<<Ridx<<" ";
	  Ridx++;
	  i+=nLen;
	}
      cx<<Ln[i];
    }
  Ln=cx.str();
  // PROCESS BRACKETS

  int brack_exists=1;
  while (brack_exists)
    {
      std::string::size_type rbrack=Ln.find(')');
      std::string::size_type lbrack=Ln.rfind('(',rbrack);  
      if (rbrack!=std::string::npos && lbrack!=std::string::npos)    
        {
	  std::string Lx=Ln.substr(lbrack+1,rbrack-lbrack-1);
	  // Check to see if a #( unit 
	  int compUnit(0);
	  while(procPair(Lx,RuleList,compUnit));
	  Ln.replace(lbrack,1+rbrack-lbrack,Lx);     
	  // Search back and find if # ( exists.
	  int hCnt;
	  for(hCnt=lbrack-1;hCnt>=0 && isspace(Ln[hCnt]);hCnt--);
	  if (hCnt>=0 && Ln[hCnt]=='#')
  	    {
	      RuleList[compUnit]=procComp(RuleList[compUnit]);
	      std::ostringstream px;
	      Ln.erase(hCnt,lbrack-hCnt);
	    }
	}
      else
	brack_exists=0;
    }
  // Do outside loop...
  int nullInt;
  while(procPair(Ln,RuleList,nullInt));      

  if (RuleList.size()!=1)
    {
      std::cerr<<"Map size not equal to 1 == "<<RuleList.size()<<std::endl;
      std::cerr<<"Error Object::ProcString : "<<Ln<<std::endl;
      exit(1);
      return 0;
    }
  TopRule=(RuleList.begin())->second;
  return 1; 
}

int
Object::interceptSurface(Geometry::Track& UT) const 
  /*!
    Given a track, fill the track with valid section
    \param UT :: Initial track 
    \return Number of segments added 
  */
{
  int cnt(0);         // Number of intesections
  // Loop over all the surfaces. 
  LineIntersectVisit LI(UT.getInit(),UT.getUVec());
  std::vector<const Surface*>::const_iterator vc;
  for(vc=SurList.begin();vc!=SurList.end();vc++)
    {
      (*vc)->acceptVisitor(LI);
      const std::vector<Geometry::Vec3D>& IPts(LI.getPoints());
      const std::vector<double>& dPts(LI.getDistance());
      for(unsigned int i=0;i<IPts.size();i++)
        {
	  if (dPts[i]>0.0)  // only interested in forward going points
	    {
	      // Is the point and enterance/exit Point
	      const int flag=calcValidType(IPts[i],UT.getUVec());
	      UT.addPoint(ObjName,flag,IPts[i]);
	      cnt++;
	    }
	}
    }
  return cnt;
}

int
Object::calcValidType(const Geometry::Vec3D& Pt,
		      const Geometry::Vec3D& uVec) const
  /*!
    Calculate if a point PT is a valid point on the track
    \param Pt :: Point to calculate from.
    \param uVec :: Unit vector of the track
    \retval 0 :: Not valid / double valid
    \retal 1 :: Entry point
    \retval -1 :: Exit Point 
   */
{
  const Geometry::Vec3D testA(Pt-uVec*OTolerance*25.0);
  const Geometry::Vec3D testB(Pt+uVec*OTolerance*25.0);
  const int flagA=isValid(testA);
  const int flagB=isValid(testB);
  if (flagA ^ flagB) return 0;
  return (flagA) ? 1 : -1;
}

int
Object::importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
		    const int singleFlag)
  /*!
    Given a distribution that has been put into an XML base set.
    The XMLcollection need to have the XMLgroup pointing to
    the section for this Object.

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
	      if (KVal=="ObjName")
		errNum=(StrFunc::convert(RPtr->getFront(),ObjName)) ? 0 : 1;
	      else if (KVal=="MatN")
		errNum=(StrFunc::convert(RPtr->getFront(),MatN)) ? 0 : 1;
	      else if (KVal=="Temperature")
		errNum=(StrFunc::convert(RPtr->getFront(),Tmp)) ? 0 : 1;
	      else if (KVal=="density")
		errNum=(StrFunc::convert(RPtr->getFront(),density)) ? 0 : 1;
	      else if (KVal=="cellList")
		errNum=procString(RPtr->getFullString());
	    }
	  if (errNum)
	    {
	      errCnt++;                 // Not good....
	      ELog::EMessages.Estream()
		<<"Object::importXML :: Failed on key: "<<KVal;
	      ELog::EMessages.report(2);
	    }
	  // Post processing
	  if (!singleFlag) 
	    SK++;
	}
    } while (!singleFlag && SK.getLevel()>=levelExit);

  return errCnt;
}


void
Object::procXML(XML::XMLcollect& XOut) const
  /*!
    Output XML schema
    \param XOut ::  XMLcollect output class
  */
{
  XOut.addComp("ObjName",ObjName);
  XOut.addComp("MatN",MatN);
  XOut.addComp("Temperature",Tmp);
  XOut.addComp("density",density);
  if (TopRule)
    XOut.addComp("cellList",TopRule->display());
  return;
}


}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid
