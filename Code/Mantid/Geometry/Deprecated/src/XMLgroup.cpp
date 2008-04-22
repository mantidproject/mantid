#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <functional>
#include <iterator>
#include <time.h>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "MantidKernel/Logger.h"
#include "AuxException.h"
#include "MantidGeometry/Matrix.h"
#include "Vec3D.h"
#include "MantidKernel/Support.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLcomp.h"
#include "XMLgrid.h"
#include "XMLnamespace.h"

namespace Mantid
{

namespace XML
{

Kernel::Logger& XMLgroup::PLog(Kernel::Logger::get("XMLgroup"));

XMLgroup::XMLgroup(XMLobject* B) : XMLobject(B)
  /*!
    Default constructor
  */
{}

XMLgroup::XMLgroup(XMLobject* B,const std::string& K) : 
  XMLobject(B,K)
  /*!
    Constructor from a string
    \param K :: Key name
   */
{}

XMLgroup::XMLgroup(XMLobject* B,const std::string& K,
		   const int GN) : 
  XMLobject(B,K,GN)
  /*!
    Constructor from a string
    \param K :: Key name
   */
{}

XMLgroup::XMLgroup(const XMLgroup& A) :
  XMLobject(A),Index(A.Index)
  /*!
    Copy constructor 
    \param A :: XMLgroup to copy
  */
{
  Grp.resize(A.Grp.size());
  transform(A.Grp.begin(),A.Grp.end(),
		Grp.begin(),std::mem_fun(&XMLobject::clone));
  for_each(A.Grp.begin(),A.Grp.end(),
	   boost::bind(&XMLobject::setParent,_1,this));
}

XMLgroup&
XMLgroup::operator=(const XMLgroup& A) 
  /*!
    Assignement operator= does a deep copy
    of the Grp vector
    \param A :: XMLgroup to copy
    \return *this
  */
{
  if (this!=&A)
    {
      XMLobject::operator=(A);
      deleteGrp();
      Grp.resize(A.Grp.size());
      transform(A.Grp.begin(),A.Grp.end(),
		Grp.begin(),std::mem_fun(&XMLobject::clone));
      for_each(A.Grp.begin(),A.Grp.end(),
	       boost::bind(&XMLobject::setParent,_1,this));
      Index=A.Index;
    }
  return *this;
}

XMLgroup*
XMLgroup::clone() const
  /*!
    \returns new (this) [ virtual constructor ] 
  */
{
  return new XMLgroup(*this);
}

XMLgroup::~XMLgroup()
  /*!
    Standard destructor
  */
{
  deleteGrp();
}

void 
XMLgroup::deleteGrp()
  /*!
    Functional to remove the memory allocated 
    for vector Grp.
  */
{
  std::vector<XMLobject*>::iterator vc;
  for(vc=Grp.begin();vc!=Grp.end();vc++)
    delete *vc;
  Grp.clear();
  Index.erase(Index.begin(),Index.end());
  return;
}

int
XMLgroup::deleteObj(XMLobject* Optr)
  /*!
    Delete an object from this group. 
    \retval 0 :: failure
    \retval 1 :: success
  */
{  
  const std::string Name=Optr->getKey();

  holdType::iterator mc=Index.find(Name);
  while(mc!=Index.end() && mc->first==Name)
    {
      std::cout<<mc->second<<std::endl;
      if (Grp[mc->second]==Optr)
        {
	  const int Icnt=mc->second;
	  delete Grp[Icnt];
	  Grp.erase(Grp.begin()+Icnt);
	  Index.erase(mc);
	  // subtract 1 from components above value
	  holdType::iterator vc;
	  for(vc=Index.begin();vc!=Index.end();vc++)
	    if (vc->second>Icnt)
	      vc->second--;

	  return 1;
	}
      mc++;
    }
  return 0;
}

XMLgroup*
XMLgroup::addGrp(const std::string& Key)
  /*!
    Adds a new key to the Grp vector as a group
    No check is carried out on the pollution of the 
    identical keys
    \param Key :: Key value
    \return pointer to the new XMLgroup
  */
{
  Index.insert(holdType::value_type(Key,Grp.size()));
  XMLgroup* X=new XMLgroup(this,Key);
  X->setDepth(depth+2);
  Grp.push_back(X);
  return X;
}


XMLobject*
XMLgroup::getItem(const int IdNum) const
  /*!
    Give the index number get a particular 
    XMLobject from vector Grp
    \param IdNum :: number of find depth
    \return XMLgroup object (0 on failure)
  */
{
  if (IdNum<0 || IdNum>=static_cast<int>(Grp.size()))
    return 0;
  return Grp[IdNum];
}

XMLobject*
XMLgroup::getObj(const std::string& KeyList,
		 const int IdNum) const
  /*!
    Given a keylist like name::aname::finalName
    it gets the XMLobject pointed to the stack
    name:aname:finalName.
    
    This is a down direction search.

    \param KeyList :: full key list
    \param Index :: number of find depth
    \return XMLgroup object (0 on failure)
  */
{
  std::string::size_type pos;
  holdType::const_iterator mc;

  // String K::A::B
  int IdCount(0);
  pos=KeyList.find("/");
  if (pos!=std::string::npos)
    {
      const std::string DX=KeyList.substr(0,pos);   // name
      const std::string nextKey=KeyList.substr(pos+1);  // aname::finalName
      
      mc=Index.find(DX);            // Find first component
      while(mc!=Index.end() && mc->first==DX)
        {
	  XMLgroup* Next = dynamic_cast<XMLgroup*>(Grp[mc->second]);
	  if (Next)
	    {
	      XMLobject* Optr=Next->getObj(nextKey);
	      if (Optr)
	        {
		  IdCount++;
		  if (IdCount>IdNum)
		    return Optr;
		}
	    }
	}
      // Failed
      return 0;
    }

  // Case where we are just looking locally:
  mc=Index.find(KeyList);   // First in multi-map:
  while(mc!=Index.end() && mc->first==KeyList)
    {
      IdCount++;
      if (IdCount>IdNum)
	return Grp[mc->second];
    }
  return 0;
}

   
XMLgroup*
XMLgroup::getGrp(const std::string& KeyList,const int IdNum) const
  /*!
    Given a keylist like name::aname::finalName
    it gets the group pointed to the stack
    name:aname. 
    \param KeyList :: full key list
    \return XMLgroup object
  */
{
  return dynamic_cast<XMLgroup*>(getObj(KeyList,IdNum));
}

XMLobject*
XMLgroup::getLastObj() const
  /*!
    \retval Last object in Grp
    \retval 0 :: Grp empty
   */
{
  return (Grp.empty() ? 0 : Grp.back());
}

template<typename T>
T*
XMLgroup::getLastType(const int cntBack) const
  /*!
    \param cntBack :: number to miss
    \retval Last object in Grp of a particular Type
    \retval 0 :: Grp empty
   */
{
  int cnt(0);
  std::vector<XML::XMLobject*>::const_reverse_iterator vc;
  for(vc=Grp.rbegin();vc!=Grp.rend();vc++)
    {
      T* Ptr=dynamic_cast<T*>(*vc);
      if (Ptr)
        {
	  if (cnt==cntBack)
	    return Ptr;
	  cnt++;
	}
    }
  return 0;
}

template<typename T>
T*
XMLgroup::getType(const int cntBack) const
  /*!
    This returns an object of the appropiate type
    designated by the template object.
    \param cntBack :: number to miss
    \retval Last object in Grp of a particular Type
    \retval 0 :: Grp container less than cntBack object of type T
   */
{
  int cnt(0);
  std::vector<XMLobject*>::const_iterator vc;
  for(vc=Grp.begin();vc!=Grp.end();vc++)
    {
      T* Ptr=dynamic_cast<T*>(*vc);
      if (Ptr)
        {
	  if (cnt==cntBack)
	    return Ptr;
	  cnt++;
	}
    }
  return 0;
}

XMLobject*
XMLgroup::findObj(const std::string& KeyName,const int IdNum) const
  /*!
    Find determines if the keyName exists in a sub-group
    off of this main group
    \param IdNum :: number to count
    \return XMLobject* on success
    \return 0 :: failed to find
  */
{
  if (KeyName.empty())
    return 0;
  // First split the keyName into Head + Tail
  std::string::size_type pos=KeyName.find("/");
  const std::string head=KeyName.substr(0,pos);
  const std::string tail=(pos!=std::string::npos) ?
    KeyName.substr(pos+1) : "";
  int activeCnt(0);
  // Iterate over each of the Object to check
  holdType::const_iterator vc;
  for(vc=Index.begin();vc!=Index.end();vc++)
    {
      if (matchPath(vc->first,head))
        {
	  if (tail.empty())
	    {
	      if (activeCnt==IdNum)
		return Grp[vc->second];
	      activeCnt++;
	    }
	  XMLgroup* Gptr=dynamic_cast<XMLgroup*>
	    (Grp[vc->second]);
	  XMLobject* Optr= 
	    (!Gptr) ? 0 : Gptr->findObj(tail);
	  int cnt(0);
	  while (Optr)
	    {
	      if (activeCnt==IdNum) return Optr;
	      cnt++;
	      activeCnt++;
	      Optr=Gptr->findObj(KeyName,cnt);
	    }
	}
      else
        {
	  XMLgroup* Gptr=dynamic_cast<XMLgroup*>
	    (Grp[vc->second]);
	  XMLobject* Optr= 
	    (!Gptr) ? 0 : Gptr->findObj(KeyName);   // get first item
	  int cnt(0);
	  while (Optr)
	    {
	      if (activeCnt==IdNum) return Optr;
	      cnt++;
	      activeCnt++;
	      Optr=Gptr->findObj(KeyName,cnt);
	    }
	}
    }
  return 0;
}

int
XMLgroup::countKey(const std::string& Key) const
  /*!
    Returns the number of keys in a group
    \param Key :: Main key or a wild card regex
  */
{
  // Iterate over each of the Object to check
  holdType::const_iterator mc;
  int count(0);
  for(mc=Index.begin();mc!=Index.end();mc++)
    if (matchPath(mc->first,Key))
      count++;
  return count;
}

template<typename T>                  // main version 
int
XMLgroup::addComp(const std::string& K,const T& V)
  /*!
    Checks component and adds object if possible
    \param K :: key value
    \param V :: value to add
  */
{
  holdType::const_iterator mc;
  const int cnt=countKey(K);    
  Index.insert(holdType::value_type(K,Grp.size()));
  
  Grp.push_back(new XMLcomp<T>(this,K,V));
  if (cnt)
    Grp.back()->setRepNum(cnt);
  Grp.back()->setDepth(depth+2);
  return 1;
}

int
XMLgroup::addComp(const std::string& K,const XMLobject* V)
  /*!
    Checks component and adds object if possible
    \param K :: key value
    \param V :: value to add (string specialisation
  */
{
  holdType::const_iterator mc;
  const int cnt=countKey(K);    
  Index.insert(holdType::value_type(K,Grp.size()));
  Grp.push_back(V->clone());
  if (cnt)
    Grp.back()->setRepNum(cnt);
  Grp.back()->setDepth(depth+2);
  return 1;
}

int
XMLgroup::addManagedObj(XMLobject* V)
  /*!
    Checks component and adds object if possible
    \param K :: key value
    \param V :: Object ot add :: It is memory managed by XMLgroup
    \retval 1 :: success
  */
{
  holdType::const_iterator mc;
  const int cnt=countKey(V->getKey());    
  Index.insert(holdType::value_type(V->getKey(),Grp.size()));
  Grp.push_back(V);
  if (cnt)
    Grp.back()->setRepNum(cnt);
  Grp.back()->setDepth(depth+2);
  return 1;
}

int
XMLgroup::readObject(std::istream& FX)
  /*!
    Given an FX stream pointed just
    after < > key
    \param FX :: input file stream
    \retval 0 :: success
    \retval -1 :: Error in parsing \<Key \>
    \retval -2 :: Close of un-open group
    \retval -3 :: Failed to find Key
    \retval -4 :: Failed to read sub-group/comp
    \retval -5 :: File ended 
  */
{
  std::string keyVal;
  std::vector<std::string> Attrib;

  XMLobject* XO;
  while(FX.good())
    {
      const int flag=XML::getNextGroup(FX,keyVal,Attrib);
      switch (flag)
        {
	case 0:             // Failed :: therefore exit
	  PLog.error("readObject:stream failure"+this->Key);
	  return -1;
	case 2:
	  if (this->Key!=keyVal)
	    {
	      PLog.error("readObject:Key mis-match (key:keyVal) "+
			 this->Key+" : "+keyVal);
	      return -1;
	    }
	  return 0;
	default:
	  XO=getObj(keyVal);
	  if (!XO)
	    {
	      PLog.error("readObject:Failed to find "+keyVal);
	      return -3;
	    }
	  if (flag<0)
	    XO->setEmpty();
	  else if (XO->readObject(FX))
	    return -4;
	}
    }
  PLog.error("readObject::End of stream key=="+this->Key);
  return -5;
}

void
XMLgroup::writeXML(std::ostream& OX) const
  /*!
    Write out writeXML schema
    \param OX :: Output stream
  */
{
  std::string::size_type pos=Key.find('%');
  std::string KeyOut( (pos!=std::string::npos)  ? Key.substr(0,pos) : Key);

  if (Grp.empty())
    {
      writeDepth(OX);
      OX<<"<"<<KeyOut<<Attr<<"/>"<<std::endl;
    }
  else
    {
      writeDepth(OX);
      OX<<"<"<<KeyOut<<Attr<<">"<<std::endl;
      std::vector<XMLobject*>::const_iterator vc;
      for(vc=Grp.begin();vc!=Grp.end();vc++)
        {
	  (*vc)->writeXML(OX);
	}
      writeDepth(OX);
      OX<<"</"<<KeyOut<<">"<<std::endl;
    }
  return;
}

const XMLobject*
XMLgroup::findParent(const XMLobject* Optr) const
  /*!
    Ugly method to get the parent group.
    Note that getParent is almost alway a lot easier and quicker.
    \param Optr :: Object to find (by pointer value)
    \retval Parent object : 0
   */
{
  std::vector<XMLobject*>::const_iterator vc;
  for(vc=Grp.begin();vc!=Grp.end();vc++)
    {
      if ((*vc)==Optr)
	return this;
      const XMLgroup* gPtr=dynamic_cast<XMLgroup*>(*vc);
      if (gPtr)
        {
	  const XMLobject* Xout=gPtr->findParent(Optr);
	  if (Xout)
	    return Xout;
	}
    }
  return 0;
}

  
}   // NAMESPACE XML

/// \cond TEMPLATE

template int XML::XMLgroup::addComp(const std::string&,const XML::nullObj&);
template int XML::XMLgroup::addComp(const std::string&,const double&);
template int XML::XMLgroup::addComp(const std::string&,const int&);
template int XML::XMLgroup::addComp(const std::string&,const std::string&);
template int XML::XMLgroup::addComp(const std::string&,const Geometry::Vec3D&);
template XML::XMLgrid<std::vector,double,std::allocator<double> >* 
XML::XMLgroup::getType(const int) const;
template XML::XMLgrid<std::vector,double,std::allocator<double> >* 
XML::XMLgroup::getLastType(const int) const;

/// \endcond TEMPLATE

}   // NAMESPACE Mantid
