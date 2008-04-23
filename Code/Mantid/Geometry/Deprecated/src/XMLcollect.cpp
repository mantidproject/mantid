#include <iostream>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <sys/stat.h>
#include <time.h>
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>

#include "MantidKernel/Logger.h"
#include "AuxException.h"
#include "MantidKernel/Support.h"
#include "MantidGeometry/RegexSupport.h"
#include "MantidGeometry/Matrix.h"
#include "Vec3D.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLcomment.h"
#include "XMLread.h"
#include "XMLcomp.h"
#include "XMLvector.h"
#include "XMLgrid.h"
#include "XMLnamespace.h"
#include "XMLcollect.h"

namespace Mantid
{

namespace XML
{

Kernel::Logger& XMLcollect::PLog(Kernel::Logger::get("XMLcollect"));

XMLcollect::XMLcollect() : 
  Master(0,"metadata_entry"),WorkGrp(&Master)
  /*!
    Constructor : Creates a top object with
    Metadata_entry 
   */
{ }

XMLcollect::XMLcollect(const XMLcollect& A) : 
  depthKey(A.depthKey),Master(A.Master),
  WorkGrp(dynamic_cast<XMLgroup*>(Master.getObj(depthKey)))
  /*!
    Standard copy constructor
    but creates a possible WorkGrp object
    from they search against the master object
  */
{}

XMLcollect&
XMLcollect::operator=(const XMLcollect& A) 
  /*!
    Standard assignment operator
    \param A :: object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      depthKey=A.depthKey;
      Master=A.Master;
      WorkGrp=dynamic_cast<XMLgroup*>(Master.getObj(depthKey));
    }
  return *this;
}

XMLcollect::~XMLcollect()
  /*!
    Standard Destructor
  */
{}

void 
XMLcollect::clear()
  /*!
    Clears everything. Done by replacement
    WorkGrp is reset.
    The depthKey emptied.
  */
{
  Master=XMLgroup(0,"metadata_entry");
  WorkGrp=&Master;
  depthKey="";
  return;
}

void
XMLcollect::closeGrp()
  /*!
    Closes the group and moves up the stack one
  */
{
  std::string::size_type pos=depthKey.rfind("/");
  if (pos!=std::string::npos)
    {
      depthKey.erase(pos);
      WorkGrp=dynamic_cast<XMLgroup*>(WorkGrp->getParent());
      if (!WorkGrp)
        {
	  WorkGrp=&Master;
	  throw ColErr::ExBase("closeGrp : failed to convert key "+depthKey);
	}
      return;
    }
  depthKey.erase(0);
  WorkGrp=&Master;
  return;
}

void
XMLcollect::addGrp(const std::string& GK)
  /*!
    Creates or finds a group with the key GK
    based on the current working group.
    Creates a name with Grp::GK
    \param GK Base/Non-base key
  */
{
  XMLgroup* Gptr=WorkGrp->addGrp(GK);
  if (Gptr)
    {
      WorkGrp=Gptr;
      if (!depthKey.empty())
	depthKey+="/";
      depthKey += GK;
    }
  return;
}

int
XMLcollect::addNumGrp(const std::string& GK)
  /*!
    Creates a group with the key GK_Num
    such that Num is the lowest positive number
    Creates a name with Grp::GK_Num.
    \param GK Base/Non-base key
  */
{
  const int index=XML::getNumberIndex(WorkGrp->getMap(),GK);
  std::ostringstream cx;
  cx<<GK<<index;

  WorkGrp=WorkGrp->addGrp(cx.str());
  if (!depthKey.empty())
    depthKey+="/";
  depthKey += GK;
  return index;
}

template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addNumComp(const std::string& Key,const V<T,A>& ContX)
  /*!
    Adds a component with a vector of things to add 
    \param K :: Key to used 
    \param ContX :: X Vector to add
    \retval Cnt of new object 
  */
{
  std::ostringstream cx;  
  cx<<Key;
  const int out = XML::getNumberIndex(WorkGrp->getMap(),cx.str());
  cx<<out;
  XMLgrid<V,T,A>* XG=new XMLgrid<V,T,A>(WorkGrp,cx.str());
  XG->setComp(0,ContX);
  WorkGrp->addManagedObj(XG);
  return out;
}


template<typename T>
int
XMLcollect::addNumComp(const std::string& K,const T& V)
  /*!
    Adds a numbered component, ie K_Num,
    were K_Num is unique, thus allowing a group of things to be added.
    \param K :: Key to used 
    \param V :: Value to add
    \retval Cnt of new object 
  */
{
  std::ostringstream cx;  
  cx<<K;
  const int out = XML::getNumberIndex(WorkGrp->getMap(),cx.str());
  cx<<out;
  WorkGrp->addComp<T>(cx.str(),V);
  return out;
}

template<>
int
XMLcollect::addNumComp(const std::string& K,const std::string& V)
  /*!
    Adds a numbered component, ie K_Num,
    were K_Num is unique, thus allowing a group of things to be added.
    \param K :: Key to used 
    \param V :: Value to add
    \retval Cnt of new object 
  */
{
  std::ostringstream cx;  
  cx<<K;
  const int out = XML::getNumberIndex(WorkGrp->getMap(),cx.str());
  cx<<out;
  WorkGrp->addComp<std::string>(cx.str(),V);
  return out;
}



template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addNumComp(const std::string& Key,const std::string& Fname,
		       const V<T,A>& ContX,const V<T,A>& ContY)
  /*!
    Adds a numbered component, ie K_Num,
    were K_Num is unique, thus allowing a group of things to be added.
    \param K :: Key to used 
    \param V :: Value to add
    \retval Cnt of new object 
  */
{
  std::ostringstream cx;  
  cx<<Key;
  const int out = XML::getNumberIndex(WorkGrp->getMap(),cx.str());
  cx<<out;
  XMLgroup* FG=WorkGrp->addGrp(cx.str());
  FG->addAttribute("file",Fname);
  StrFunc::writeFile(Fname,ContX,ContY);
  return out;
}

template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addNumComp(const std::string& Key,const V<T,A>& ContX,
		       const V<T,A>& ContY)
  /*!
    Adds a numbered component, ie K_Num,
    were K_Num is unique, thus allowing a group of things to be added.
    \param K :: Key to used 
    \param V :: Value to add
    \retval Cnt of new object 
  */
{
  std::ostringstream cx;  
  cx<<Key;
  const int out = XML::getNumberIndex(WorkGrp->getMap(),cx.str());
  cx<<out;
  WorkGrp->addManagedObj(new XMLvector<V,T,A>(WorkGrp,cx.str(),ContX,ContY));
  return out;
}

template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addNumComp(const std::string& Key,const V<T,A>& ContX,
		       const V<T,A>& ContY,const V<T,A>& ContZ)
  /*!
    Adds a numbered component, ie K_Num,
    were K_Num is unique, thus allowing a group of things to be added.
    \param K :: Key to used 
    \param V :: Value to add
    \param ContX :: Container object to add (first)
    \param ContY :: Container object to add (second)
    \param ContZ :: Container object to add (third)
    \retval Cnt of new object 
  */
{
  std::ostringstream cx;  
  cx<<Key;
  const int out = XML::getNumberIndex(WorkGrp->getMap(),cx.str());
  cx<<out;
  WorkGrp->addComp(cx.str(),XMLvector<V,T,A>(WorkGrp,Key,ContX,ContY,ContZ));
  return out;
}

template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addNumComp(const std::string& Key,const std::string& Fname,
		       const V<T,A>& ContX,const V<T,A>& ContY,
		       const V<T,A>& ContZ)
  /*!
    Adds a numbered component, ie K_Num,
    were K_Num is unique, thus allowing a group of things to be added.
    \param Key :: Key to used 
    \param ContX :: Vector component
    \retval Cnt of new object 
  */
{
  std::ostringstream cx;  
  cx<<Key;
  const int out = XML::getNumberIndex(WorkGrp->getMap(),cx.str());
  cx<<out;
  XMLgroup* FG=WorkGrp->addGrp(cx.str());
  FG->addAttribute("file",Fname);
  StrFunc::writeFile(Fname,ContX,ContY,ContZ);
  return out;
}
// ----------------------------
//         ADDCOMP  :FILE:
// ---------------------------


template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addComp(const std::string& Key,const std::string& Fname,
		    const V<T,A>& ContX, const V<T,A>& ContY)
  /*!
    Sophisticated container adder: It takes two names and creates a file
    of the data, and then puts the filename into the object. 
    \param Key :: Key value to use
    \param Fname :: File to output 
  */
{
  StrFunc::writeFile(Fname,ContX,ContY);
  const int retVal=WorkGrp->addComp<std::string>(Key,Fname);
  XMLgroup* FG=WorkGrp->addGrp(Key);
  FG->addAttribute("file",Fname);
  StrFunc::writeFile(Fname,ContX,ContY);
  return retVal;
}

template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addComp(const std::string& Key,const std::string& Fname,
		    const V<T,A>& ContX, const V<T,A>& ContY,
		    const V<T,A>& ContZ)
  /*!
    Sophisticated container adder: It takes two names and creates a file
    of the data, and then puts the filename into the object. 
    \param Key :: Key value to use
    \param Fname :: File to output 
    \param ContX :: Vector component to add
    \param ContY :: Vector component to add
    \param ContZ :: Vector component to add
  */
{
  StrFunc::writeFile(Fname,ContX,ContY);
  const int retVal=WorkGrp->addComp<std::string>(Key,Fname);
  XMLgroup* FG=WorkGrp->addGrp(Key);
  FG->addAttribute("file",Fname);
  StrFunc::writeFile(Fname,ContX,ContY);
  return retVal;
}

// ------------------------
// ADDCOMP
// ------------------------

template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addComp(const std::string& Key,const V<T,A>& ContX)
  /*!
    Adds a component with a vector of things to add 
    \param K :: Key to used 
    \param ContX :: X Vector to add
    \retval Cnt of new object 
  */
{
  XMLgrid<V,T,A>* XG=new XMLgrid<V,T,A>(WorkGrp,Key);
  XG->setComp(0,ContX);
  return WorkGrp->addManagedObj(XG);
  return 0;
}

template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addComp(const std::string& Key,const V<T,A>& ContX,
		    const V<T,A>& ContY)
  /*!
    Adds a component with a vector of things to add 
    \param K :: Key to used 
    \param ContX :: X Vector to add
    \param ContY :: Y Vector to add
    \retval Cnt of new object 
  */
{
  return WorkGrp->addManagedObj(new XMLvector<V,T,A>(WorkGrp,Key,ContX,ContY));
}

template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addComp(const std::string& Key,const V<T,A>& ContX,
		    const V<T,A>& ContY,const V<T,A>& ContZ)
  /*!
    Adds a component with a vector of things to add 
    \param K :: Key to used 
    \param ContX :: X Vector to add
    \param ContY :: Y Vector to add
    \param ContZ :: Error Vector to add
    \retval Cnt of new object 
  */
{
  XMLgrid<V,T,A>* XG=new XMLgrid<V,T,A>(WorkGrp,Key);
  XG->setComp(0,ContX);
  XG->setComp(1,ContY);
  XG->setComp(2,ContZ);
  return WorkGrp->addManagedObj(XG);
}

template<template<typename T,typename A> class V,typename T,typename A>
int
XMLcollect::addComp(const std::string& Key,const V<T,A>& ContA,
		    const V<T,A>& ContB,const V<T,A>& ContC,
		    const V<T,A>& ContD)
  /*!
    Adds a component with a vector of things to add 
    \param K :: Key to used 
    \param ContA :: X Vector to add
    \param ContB :: Y Vector to add
    \param ContC :: Y2 Vector to add
    \param ContD :: Y3 Vector to add
    \retval Cnt of new object 
  */
{
  XMLgrid<V,T,A>* XG=new XMLgrid<V,T,A>(WorkGrp,Key);
  XG->setComp(0,ContA);
  XG->setComp(1,ContB);
  XG->setComp(2,ContC);
  XG->setComp(3,ContD);
  return WorkGrp->addManagedObj(XG);
}

template<typename T>
int
XMLcollect::addComp(const std::string& K,const T& V)
  /*!
    \param K :: Key to used
    \param V :: Value to add
    \retval 0 if key already exists 
    \retval 1 new key object added
  */
{
  return WorkGrp->addComp<T>(K,V);
}

template<>
int
XMLcollect::addComp(const std::string& K,const std::string& V)
  /*!
    \param K :: Key to used
    \param V :: Value to add
    \retval 0 if key already exists 
    \retval 1 new key object added
  */
{
  return WorkGrp->addComp<std::string>(K,V);
}

// ------------------------------------------------------------

void
XMLcollect::addComment(const std::string& Line)
  /*!
    Create a comment with line 
    \param Line :: Line object to add
  */
{
  XMLcomment* CPtr=new XMLcomment(WorkGrp,Line);
  WorkGrp->addManagedObj(CPtr);
  return;
}

void
XMLcollect::addAttribute(const std::string& K,const char* V)
  /*!
    Adds an attribute to the currently opened group
    \param K :: Key to used
    \param V :: Value to add (must be convertable by operator<<)
  */
{
  WorkGrp->addAttribute(K,std::string(V));
  return;
}

template<typename T>
void
XMLcollect::addAttribute(const std::string& K,const T& V)
  /*!
    \param K :: Key to used
    \param V :: Value to add (must be convertable by operator<<)
  */
{
  std::ostringstream cx;
  cx<<V;
  WorkGrp->addAttribute(K,cx.str());
  return;
}

template<>
void
XMLcollect::addAttribute(const std::string& K,const std::string& V)
  /*!
    \param K :: Key to used
    \param V :: Value to add
  */
{
  WorkGrp->addAttribute(K,V);
  return;
}

template<typename T>
void
XMLcollect::addAttribute(const std::string& Comp,
			 const std::string& K,const T& V)
  /*!
    Add an attribute to a component. 
    - E.g. readType=file
    - K == readType , V == file
    \param Comp :: Compenent Key to use
    \param K :: Key to used (primary tag)
    \param V :: Value to add (secondary tag)
  */
{
  XMLobject* Optr=WorkGrp->getObj(Comp);
  if (Optr)
    {
      std::ostringstream cx;
      cx<<V;
      Optr->addAttribute(K,cx.str());
    }
  else
    {
      std::ostringstream cx;
      cx<<"Error getting XMLcollect::addAttribute:"+Comp+
	" from Workgroup "<<*WorkGrp;
      PLog.error(cx.str());
    }
  return;
}

void
XMLcollect::addAttribute(const std::string& Comp,
			 const std::string& K,const char* V)
  /*!
    Add an attribute to a component. 
    - E.g. readType=file
    - K == readType , V == file
    \param Comp :: Compenent Key to use
    \param K :: Key to used (primary tag)
    \param V :: Value to add (secondary tag)
  */
{
  XMLobject* Optr=WorkGrp->getObj(Comp);
  if (Optr)
    Optr->addAttribute(K,std::string(V));
  else
    {
      std::cerr<<"Error getting XMLcollect::addAttribute:"<<Comp<<std::endl;
      std::cerr<<"WorkGrp = "<<*WorkGrp<<std::endl;
    }
  return;
}

int
XMLcollect::addComp(const std::string& K,const char* V)
  /*!
    \param K :: Key to used
    \param V :: Value to add
    \retval 0 if key already exists 
    \retval 1 new key object added
  */
{
  std::string Out(V);
  return addComp<std::string>(K,Out);
}

int
XMLcollect::addNumComp(const std::string& K,const char* V)
  /*!
    \param K :: Key to used
    \param V :: Value to add
    \retval 0 if key already exists 
    \retval 1 new key object added
  */
{
  std::string Out(V);
  return addNumComp<std::string>(K,Out);
}

const XMLobject*
XMLcollect::findParent(const XMLobject* Optr) const
  /*!
    This is an ugly function to determine the parent
    of the Optr. Top down search
   */
{
  if (Optr==0 || Optr==&Master)  // No parent
    return 0;
  return Master.findParent(Optr);
}

int
XMLcollect::deleteObj(XMLobject* Optr)
  /*!
    This delete a particular object from the system
    \param  Optr :: object to delete
    \retval 1 :: success [object deleted]
    \retval 0 :: failure
  */
{
  XMLgroup* Parent=(Optr) ? 
    dynamic_cast<XMLgroup*>(Optr->getParent()) : 0;
    
  return (Parent) ? Parent->deleteObj(Optr) : 0;
}

std::string
XMLcollect::makeTimeString(const tm* TimePtr) const
  /*!
    Converts a unix clock (from gmtime (struc tm poiter)
    to a XML date format
    
    \return Time string
  */
{
  std::ostringstream cx;
  cx<<1900+TimePtr->tm_year<<"-";
  cx.fill('0');
  cx.width(2);
  cx<<TimePtr->tm_mon+1<<"-";
  cx.width(2);
  cx<<TimePtr->tm_mday;
  // time
  cx<<" ";
  cx<<std::setw(2)<<TimePtr->tm_hour<<":";
  cx<<std::setw(2)<<TimePtr->tm_min<<":";
  cx<<std::setw(2)<<TimePtr->tm_sec;
  return cx.str();
}

int
XMLcollect::readObject(const std::string& Fname)
  /*!
    First finds the first instance of the Grp 
    in the file. 

    Determines if the GrpName is in this group
    if not creates a group / clears group

    Re-reads the file
    \param FName :: 
    \param GrpName :: Name of the group
    \retval -1 :: Error with file 
    \retval -2 :: Unable to read GrpName
    \retval 0 :: Success
  */
{
  std::string Null;
  return readObject(Fname,Null);
}

int
XMLcollect::readObject(const std::string& Fname,
		       const std::string& GrpName)
  /*!
    First finds the first instance of the Grp 
    in the file. 

    Determines if the GrpName is in this group
    if not creates a group / clears group

    Re-reads the file
    \param FName :: 
    \param GrpName :: Name of the group
    \retval -1 :: Error with file 
    \retval -2 :: Unable to read GrpName
    \retval 0 :: Success
  */
{
  // First Open file and find object
  std::ifstream FX(Fname.c_str());
  if (!FX.good())
    return -1;
  return readObject(FX,GrpName);
}

int 
XMLcollect::readObject(std::istream& FX,const std::string& KeyName)
  /*!
    Take filename + Key (if not a key then use Master)
    - Exits after < key > has been read
    \param FX :: File stream
    \param KeyName :: Keyname to search
    \retval -ve on error
    \retval 0 :: success
  */
{
  // Read from Master::
  if (KeyName.empty())
    {
      if (XML::getFilePlace(FX,Master.getKey())!=1)
	return -1;
      return Master.readObject(FX);
    }
  // Read from a key ::
  if (XML::getFilePlace(FX,KeyName)!=1)
    return -1;
  //
  // Clear Count here :
  //
  XMLobject* Xptr = Master.findObj(KeyName);
  if (!Xptr)
    return -2;
  return Xptr->readObject(FX);
}

XMLobject*
XMLcollect::getObj(const std::string& KeyName,
		   const int IdNum) const
  /*!
    Given a key name find the closest match 
    \param KeyName :: Name to search
    \param IdNum :: Which match to return
    \return XMLobject
  */
{
  return Master.getObj(KeyName,IdNum);
}

XMLobject*
XMLcollect::findObj(const std::string& KeyName,
		   const int IdNum) const
  /*!
    Given a key name find the closest match 
    unlike getObj it allows to descend to search.
    \param KeyName :: Name to search
    \param IdNum :: Which match to return
    \return XMLobject ptr (0 on failure)
  */
{
  return Master.findObj(KeyName,IdNum);
}

int
XMLcollect::setToKey(const std::string& KeyName,const int IdNum)
  /*!
    Given a Key and a number in indents to find
    determine if the key is to a group and set the 
    value to thate group/object.
    \param KeyName :: Name of the Key/Regex to find
    \param IdNumm :: IndexNumber
    \retval -1 :: Object not found
    \retval -2 :: Object not a group
    \retval 0 :: success
  */
{
  XMLobject* XObj=Master.findObj(KeyName,IdNum);
  if (!XObj) // Not found
    return -1;
  XMLgroup* XGrp=dynamic_cast<XMLgroup*>(XObj);
  if (!XGrp)
    {
      XGrp=dynamic_cast<XMLgroup*>(XObj->getParent());
      if (!XGrp)
	throw ColErr::ExBase("setToKey::Error converting from XMLobject to XMLgroup");
      WorkGrp=XGrp;
      return -2;
    }
      
  WorkGrp=XGrp;
  return 0;
}

int
XMLcollect::loadXML(const std::string& Fname)
  /*!
    Takes a file and reads data from the files
    This is a two pass system:
    - First to get the groups
    - Second to get XMLread object
    \param Fname :: File to open
  */
{
  std::ifstream IX;
  IX.open(Fname.c_str());
  return loadXML(IX,"",std::vector<std::string>());
}

int
XMLcollect::loadXML(const std::string& FName,const std::string& Key)
  /*!
    Given a key: load from the key.
    \param FName :: Filename
    \retval -1 :: Faile to get open group
    \retval -2 :: Failed to get file
    \todo Convert Key to XPath
  */
{
  if (FName.empty())
    return -2;

  std::ifstream IX;
  IX.open(FName.c_str());
  
  std::string XKey;
  std::vector<std::string> Attrib;

  int flag(1);
  while(flag)
    {
      flag=XML::getNextGroup(IX,XKey,Attrib);         // at start of group
      if (XKey==Key)
        {
	  if (flag==2)          // Closed group and we haven't opened one!
	    return -1;
	  if (flag==-1)         // No work to do
	    {
	      XMLcomp<nullObj>* NPtr=new XMLcomp<nullObj>(WorkGrp,XKey);
	      NPtr->addAttribute(Attrib);
	      WorkGrp->addManagedObj(NPtr);
	      return 0;
	    }
	  
	  return loadXML(IX,Key,Attrib);
	}
    }
  // Failed to get file/ file at end
  return -2;
}

int
XMLcollect::loadXML(std::istream& IX,const std::string& CKey,
		    const std::vector<std::string>& VAttrib)
  /*!
    Process from this group onwards
    This assumes we have just opened a group with
    XKey and Attribute.
    \param CKey :: closing Key
  */
{
  XML::XMLread* RPtr;

  int endCnt(1);                     // how many open we have had
  int flag(1);                       // Assume that we have just opened

  std::string XKey(CKey);

  std::vector<std::string> Attrib(VAttrib);
  if (XKey=="metadata_entry")
    flag+=10;

  // collect the buffer to the next object ?
  //  std::vector<std::string> buffer;
  //  XML::collectBuffer(IX,buffer);

  int flagB;
  std::vector<std::string> Data;
  while (IX.good() && flag && endCnt)
    {
      // New open group
      std::vector<std::string> Secondary;
      std::string XKeyB;
      std::string closeKey;  // Key closed with 
      
      // Note that Secondary and Data are cleared at each call
      flagB= (flag==1) ?
	XML::getGroupContent(IX,XKeyB,Secondary,Data) : 
	XML::getNextGroup(IX,XKeyB,Secondary);
	
      if (flag==-1)            // Null group
        {
	  XMLcomp<nullObj>* NPtr=new XMLcomp<nullObj>(WorkGrp,XKey);        // Null group 
	  NPtr->addAttribute(Attrib);
	  WorkGrp->addManagedObj(NPtr);
	}
      else if (flagB==2)          // closed key
        {
	  if (flag==1 && XKeyB==XKey)   // Close a REAL Object
	    {
	      RPtr=new XMLread(WorkGrp,XKey);
	      RPtr->addAttribute(Attrib);
	      RPtr->setObject(Data);
	      WorkGrp->addManagedObj(RPtr);
	    }
	  else if (flag==-1)              // Close a Group 
	    {
	      RPtr=new XMLread(WorkGrp,XKey);
	      RPtr->addAttribute(Attrib);
	      WorkGrp->addManagedObj(RPtr);	      
	      closeGrp();
	    }
	  else if (flag==2)
	    {
	      closeGrp();
	    }
	}
      else if (flag==1 && 
	       (flagB==1 || flagB==-1))                               // A New group
        {
	  if (CKey==XKey)             // open a new group
	    endCnt++;

	  addGrp(XKey);
	  WorkGrp->addAttribute(Attrib);
	}

      // Matching for range
      if (flag==2 && CKey==XKey)          // Close a group and Key matches
	endCnt--;
	

      flag=flagB;
      Attrib=Secondary;
      XKey=XKeyB;
    }

  return 0;
}

void
XMLcollect::writeXML(std::ostream& OX) const
  /*!
    Accessor to writeXML from Master
    \param OX :: output stream 
  */
{
  OX<<"<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>"<<std::endl;
  Master.writeXML(OX);
  return;
}

}   // NAMESPACE Geometry 

/*!
\cond TEMPLATE
*/

template void XML::XMLcollect::addAttribute(const std::string&,const double&);
template void XML::XMLcollect::addAttribute(const std::string&,const int&);

template void XML::XMLcollect::addAttribute(const std::string&,const std::string&,const double&);

template int XML::XMLcollect::addComp(const std::string&,const XML::nullObj&);
template int XML::XMLcollect::addComp(const std::string&,const double&);
template int XML::XMLcollect::addComp(const std::string&,const int&);
template int XML::XMLcollect::addComp(const std::string&,const std::vector<double>&);
template int XML::XMLcollect::addComp(const std::string&,const std::vector<int>&);
template int XML::XMLcollect::addComp(const std::string&,const std::vector<std::string>&);
template int XML::XMLcollect::addComp(const std::string&,const std::string&,
				      const std::vector<double>&,const std::vector<double>&);
template int XML::XMLcollect::addComp(const std::string&,const std::vector<double>&,
				      const std::vector<double>&);
template int XML::XMLcollect::addComp(const std::string&,const std::vector<double>&,
				      const std::vector<double>&,const std::vector<double>&);

template int XML::XMLcollect::addNumComp(const std::string&,const XML::nullObj&);
template int XML::XMLcollect::addNumComp(const std::string&,const double&);
template int XML::XMLcollect::addNumComp(const std::string&,const int&);
template int XML::XMLcollect::addNumComp(const std::string&,const std::string&,
				      const std::vector<double>&,const std::vector<double>&);

#ifdef Geometry_Vec3D_h
template int XML::XMLcollect::addComp(const std::string&,const Geometry::Vec3D&);
template int XML::XMLcollect::addNumComp(const std::string&,const Geometry::Vec3D&);
#endif


/*!
\endcond TEMPLATE
*/

} // NAMESPACE Mantid
