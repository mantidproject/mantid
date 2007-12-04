#include <fstream>
#include <iomanip>
#include <iostream>
#include <complex>
#include <cmath>
#include <vector>
#include <map>
#include <list>
#include <stack>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "MantidKernel/Logger.h"
#include "AuxException.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "Triple.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Track.h"
#include "Line.h"
#include "BaseVisit.h"
#include "Surface.h"
#include "Rules.h"
#include "Object.h"

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Intersection::PLog(Kernel::Logger::get("Intersection"));

Intersection::Intersection() : Rule(),
  A(0),B(0)
  /*!
    Standard Constructor with null leaves
  */
{}

Intersection::Intersection(Rule* Ix,Rule* Iy) : Rule(),
  A(Iy),B(Ix)
  /*!
    Intersection constructor from two Rule ptrs.
    - Sets A,B's parents to *this
    - Allowed to control Ix and Iy
    \param Ix :: Rule A
    \param Iy :: Rule B 
  */
{ 
  if (A)
    A->setParent(this);
  if (B)
    B->setParent(this); 
}

Intersection::Intersection(Rule* Parent,Rule* Ix,Rule* Iy) : 
  Rule(Parent),A(Ix),B(Iy)    
  /*!
    Intersection constructor from two Rule ptrs 
    - Sets A,B's parents to *this.
    \param Parent :: Set the Rule::Parent pointer
    \param Ix :: Rule A
    \param Iy :: Rule B 
  */
{
  if (A)
    A->setParent(this);
  if (B)
    B->setParent(this);
}

Intersection::Intersection(const Intersection& Iother) : 
  Rule(),A(0),B(0)
  /*!
    Copy constructor:
    Does a clone on the sub-tree below
    \param Iother :: Intersection to copy
  */
{
  if (Iother.A)
    {
      A=Iother.A->clone();
      A->setParent(this);
    }
  if (Iother.B)
    {
      B=Iother.B->clone();
      B->setParent(this);
    }
}

Intersection&
Intersection::operator=(const Intersection& Iother) 
  /*!
    Assignment operator :: Does a deep copy
    of the leaves of Iother. 
    \param Iother :: object to copy
    \returns *this
  */
{
  if (this!=&Iother)
    {
      Rule::operator=(Iother);
      // first create new copy all fresh
      if (Iother.A)
        {
	  Rule* Xa=Iother.A->clone();
	  delete A;
	  A=Xa;
	  A->setParent(this);
	}
      if (Iother.B)
        {
	  Rule* Xb=Iother.B->clone();
	  delete B;
	  B=Xb;
	  B->setParent(this);
	}
    }
  return *this;
}

Intersection::~Intersection()
  /*!
    Destructor :: responsible for the two
    leaf intersections.
  */
{
  delete A;
  delete B;
}

Intersection*
Intersection::clone() const
  /*!
    Virtual copy constructor
    \returns new Intersection(this)
  */
{
  return new Intersection(*this);
}

int
Intersection::isComplementary() const 
  /*!
    Determine is the rule has complementary
    sub components
    \retval 1 :: A side
    \retval -1 :: B side
    \retval 0 :: no complement
  */
{
  if (A && A->isComplementary())
    return 1;
  if (B && B->isComplementary())
    return -1;

  return 0;
}

void
Intersection::setLeaves(Rule* aR,Rule* bR)
  /*!
    Replaces a both with a rule.
    No deletion is carried out but sets the parents.
    \param aR :: Rule on the left
    \param bR :: Rule on the right
  */
{
  A=aR;
  B=bR;
  if (A)
    A->setParent(this);
  if (B)
    B->setParent(this);
  return;
}

void
Intersection::setLeaf(Rule* nR,const int side)
  /*!
    Replaces a leaf with a rule.
    No deletion is carried out
    \param nR :: new rule
    \param side :: side to use 
    - 0 == LHS 
    - true == RHS
  */
{
  if (side)
    {
      B=nR;
      if (B)
	B->setParent(this);
    }
  else
    {
      A=nR;
      if (A)
	A->setParent(this);
    }
  return;
}

int 
Intersection::findLeaf(const Rule* R) const
  /*!
    Finds out if the Rule is the
    same as the leaves
    \param R :: Rule pointer to compare
    \retval 0 / 1 for LHS / RHS leaf 
    \retval -1 :: neither leaf
  */
{
  if (A==R)
    return 0;
  if (B==R)
    return 1;
  return -1;
}

Rule*
Intersection::findKey(const int KeyN)
  /*!
    Finds the leaf with the surface number KeyN
    \param KeyN :: Number to search for
    \retval 0 :: no leaf with that key number availiable
    \retval Rule* if an appropiate leaf is found
  */
{
  Rule* PtrOut=(A) ? A->findKey(KeyN) : 0;
  if (PtrOut)
    return PtrOut;
  return (B) ? B->findKey(KeyN) : 0;
}

std::string
Intersection::display() const
  /*!
    Displaces a bracket wrapped object
    \return Bracketed string
  */
{
  std::string out;
  if (!A || !B)
    throw ColErr::ExBase(2,"Intersection::display incomplete type");
  if (A->type()==-1)
    out="("+A->display()+")";
  else
    out=A->display();

  out+=" ";
  
  if (B->type()==-1)
    out+="("+B->display()+")";
  else
    out+=B->display();
  return out;
}

std::string
Intersection::displayAddress() const
  /*!
    Debug function that converts the 
    the intersection ion space delimited unit
    to denote intersection.
    \returns ( leaf leaf )
  */
{
  std::stringstream cx;
  cx<<" [ "<<reinterpret_cast<long>(this);
  if (A && B)
    cx<<" ] ("+A->displayAddress()+" "+B->displayAddress()+") ";
  else if (A)
    cx<<" ] ("+A->displayAddress()+" 0x0 ) ";
  else if (B)
    cx<<" ] ( 0x0 "+B->displayAddress()+") ";
  else
    cx<<" ] ( 0x0 0x0 ) ";
  return cx.str();
}

int
Intersection::isValid(const Geometry::Vec3D& Vec) const
  /*!
    Calculates if Vec is within the object
    \param Vec :: Point to test
    \retval 1 ::  Vec is within object 
    \retval 0 :: Vec is outside object.
  */
{
  if (!A || !B)
    return 0;
  return (A->isValid(Vec) && B->isValid(Vec)) ? 1 : 0;
}

int
Intersection::isValid(const std::map<int,int>& MX) const
  /*!
    Use MX to determine if the surface truth etc is 
    valie
    \param MX :: map of key + logical value XOR sign
    \retval 1 ::  Both sides are valid
    \retval 0 :: Either side is invalid.
  */
{
  if (!A || !B)
    return 0;
  return (A->isValid(MX) && B->isValid(MX)) ? 1 : 0;
}

int
Intersection::simplify() 
  /*!
    Union simplification:: 
      - -S S simplify to True.
      - S S simplify to S 
      - -S -S simplifies to -S
      \retval 1 if clauses removed (not top)
      \retval -1 replacement of this intersection is required
                  by leaf 0
      \retval 0 if no work to do.
  */
{
  return 0;
}

// -------------------------------------------------------------
//         UNION
//---------------------------------------------------------------

Kernel::Logger& Union::PLog(Kernel::Logger::get("Union"));

Union::Union() : 
  Rule(),A(0),B(0)
  /*!
    Standard Constructor with null leaves
  */
{}

Union::Union(Rule* Parent,Rule* Ix,Rule* Iy) : Rule(Parent),
  A(Ix),B(Iy)    
  /*!
    Union constructor from two Rule ptrs.
    - Sets A,B's parents to *this
    - Allowed to control Ix and Iy
    \param Parent :: Rule that is the parent to this 
    \param Ix :: Rule A
    \param Iy :: Rule B 
  */
{
  if (A)
    A->setParent(this);
  if (B)
    B->setParent(this);
}

Union::Union(Rule* Ix,Rule* Iy) : Rule(),
  A(Ix),B(Iy)
  /*!
    Union constructor from two Rule ptrs 
    - Sets A,B's parents to *this.
    - Allowed to control Ix and Iy
    \param Ix :: Rule A
    \param Iy :: Rule B 
  */
{
  if (A)
    A->setParent(this);
  if (B)
    B->setParent(this);
}

Union::Union(const Union& Iother) : Rule(Iother),
   A(0),B(0)
  /*!
    Copy constructor:
    Does a clone on the sub-tree below
    \param Iother :: Union to copy
  */
{
  if (Iother.A)
    {
      A=Iother.A->clone();
      A->setParent(this);
    }
  if (Iother.B)
    {
      B=Iother.B->clone();
      B->setParent(this);
    }
}

Union&
Union::operator=(const Union& Iother) 
  /*!
    Assignment operator :: Does a deep copy
    of the leaves of Iother. 
    \param Iother :: Union to assign to it. 
    \returns this union (copy).
  */
{
  if (this!=&Iother)
    {
      Rule::operator=(Iother);
      // first create new copy all fresh
      if (Iother.A)
        {
	  Rule* Xa=Iother.A->clone();
	  delete A;
	  A=Xa;
	  A->setParent(this);
	}
      if (Iother.B)
        {
	  Rule* Xb=Iother.B->clone();
	  delete B;
	  B=Xb;
	  B->setParent(this);
	}
    }
  return *this;
}

Union::~Union()
  /*!
    Delete operator : deletes both leaves
  */
{
  delete A;
  delete B;
}

Union*
Union::clone() const
  /*!
    Clone allows deep virtual coping
    \returns new Union copy.
  */
{
  return new Union(*this);
}

void
Union::setLeaf(Rule* nR,const int side)
  /*!
    Replaces a leaf with a rule.
    No deletion is carried out
    \param nR :: new rule
    \param side :: side to use 
    - 0 == LHS 
    - true == RHS
  */
{
  if (side)
    {
      B=nR;
      if (B)
	B->setParent(this);
    }
  else
    {
      A=nR;
      if (A)
	A->setParent(this);
    }
  return;
}

void
Union::setLeaves(Rule* aR,Rule* bR)
  /*!
     Replaces a both with a rule.
    No deletion is carried out but sets the parents.
    \param aR :: Rule on the left
    \param bR :: Rule on the right
  */
{
  A=aR;
  B=bR;
  if (A)
    A->setParent(this);
  if (B)
    B->setParent(this);
  return;
}

int 
Union::findLeaf(const Rule* R) const
  /*!
    Finds out if the Rule is the
    same as the leaves
    \param R :: Rule pointer to compare
    \retval 0 / 1 for LHS / RHS leaf 
    \retval -1 :: neither leaf
  */
{
  if (A==R)
    return 0;
  if (B==R)
    return 1;
  return -1;
}

Rule*
Union::findKey(const int KeyN)
  /*!
    Finds the leaf with the surface number KeyN
    \param KeyN :: Number to search for
    \retval 0 :: no leaf with that key number availiable
    \retval Rule* if an appropiate leaf is found
  */
{

  Rule* PtrOut=(A) ? A->findKey(KeyN) : 0;
  if (PtrOut)
    return PtrOut;
  return (B) ? B->findKey(KeyN) : 0;
}

int
Union::isComplementary() const 
  /*!
    Determine is the rule has complementary
    sub components
    \retval 1 :: A side
    \retval -1 :: B side
    \retval 0 :: no complement
  */
{
  if (A && A->isComplementary())
    return 1;
  if (B && B->isComplementary())
    return -1;

  return 0;
}

int
Union::simplify() 
  /*!
    Union simplification:: 
      - -S S simplify to True.
      - S S simplify to S 
      - -S -S simplifies to -S
      \retval 1 if clauses removed (not top)
      \retval -1 replacement of this intersection is required
                  by leaf 0
      \retval 0 if no work to do.
  */
{
  if (!commonType())
    return 0;
  return 0;
}

int
Union::isValid(const Geometry::Vec3D& Vec) const
  /*!
    Calculates if Vec is within the object
    \param Vec :: Point to test
    \retval  1 ::  Vec is within object 
    \retval 0 :: Vec is outside object.
  */
{
  return ((A && A->isValid(Vec)) ||
	  (B && B->isValid(Vec))) ? 1 : 0;
}

int
Union::isValid(const std::map<int,int>& MX) const
  /*!
    Use MX to determine if the surface truth etc is 
    valie
    \param MX :: map of key + logical value XOR sign
    \retval 1 :: if either side is valid
    \retval 0 :: Neither side is valid
  */
{
  return ((A && A->isValid(MX)) ||
	  (B && B->isValid(MX))) ? 1 : 0;
}


std::string
Union::display() const
  /*!
    Display the union in the form
    (N:M) where N,M are the downward
    rules
    \returns bracket string 
  */
{
  std::string out;
  if (!A || !B)
    throw ColErr::ExBase(2,"Intersection::display incomplete type");
  if (A->type()==1)
    out="("+A->display()+")";
  else
    out=A->display();

  out+=" : ";
  
  if (B->type()==1)
    out+="("+B->display()+")";
  else
    out+=B->display();

  return out;
}

std::string
Union::displayAddress() const
  /*!
    Returns the memory address as 
    a string. Displays addresses of leaves
    \returns String of address
  */
{
  std::stringstream cx;
  cx<<" [ "<<reinterpret_cast<long int>(this);

  if (A && B)
    cx<<" ] ("+A->displayAddress()+" : "+B->displayAddress()+") ";
  else if (A)
    cx<<" ] ("+A->displayAddress()+" : 0x0 ) ";
  else if (B)
    cx<<" ] ( 0x0 : "+B->displayAddress()+") ";
  else
    cx<<" ] ( 0x0 : 0x0 ) ";
  return cx.str();
}


// -------------------------------------------------------------
//         SURF KEYS
//---------------------------------------------------------------
Kernel::Logger& SurfPoint::PLog(Kernel::Logger::get("SurfPoint"));

SurfPoint::SurfPoint() : Rule(),
  key(0),keyN(0),sign(1)
  /*!
    Constructor with null key/number
  */
{}

SurfPoint::SurfPoint(const SurfPoint& A) : Rule(),
  key(A.key),keyN(A.keyN),sign(A.sign)
  /*!
    Copy constructor
    \param A ::SurfPoint to copy
  */
{}

SurfPoint*
SurfPoint::clone() const
  /*!
    Clone constructor
    \return new(*this)
  */
{
  return new SurfPoint(*this);
}


SurfPoint&
SurfPoint::operator=(const SurfPoint& A) 
  /*!
    Assigment operator
    \param A :: Object to copy
    \returns *this
  */
{
  if (&A!=this)
    {
      key=A.key;
      keyN=A.keyN;
      sign=A.sign;
    }
  return *this;
}

SurfPoint::~SurfPoint()
  /*!
    Destructor
  */
{}

void
SurfPoint::setLeaf(Rule* nR,const int)
  /*!
    Replaces a leaf with a rule.
    This REQUIRES that nR is of type SurfPoint
    \param nR :: new rule
    \param int :: ignored
  */
{
  std::cerr<<"Calling SurfPoint setLeaf"<<std::endl;
  SurfPoint* newX = dynamic_cast<SurfPoint*>(nR);
  if (newX)
    *this = *newX;
  return;
}

void
SurfPoint::setLeaves(Rule* aR,Rule*)
  /*!
    Replaces a leaf with a rule.
    This REQUIRES that nR is of type SurfPoint
    \param aR :: new rule
  */
{
  std::cerr<<"Calling SurfPoint setLeaf"<<std::endl;
  SurfPoint* newX = dynamic_cast<SurfPoint*>(aR);
  if (newX)
    *this = *newX;
  return;
}

int
SurfPoint::findLeaf(const Rule* A) const
  /*!
    Determines if this rule is a particular leaf value
    uses memory address to compare.
    \param A :: Rule to check
    \returns 0 if true and -1 if false.
  */
{
  return (this==A) ? 0 : -1;
}

Rule*
SurfPoint::findKey(const int KeyNum)
  /*!
    Finds the leaf with the surface number KeyN
    \param KeyNum :: Number to search for
    \retval 0 :: no leaf with that key number availiable
    \retval Rule* if an appropiate leaf is found
  */
{
  return (KeyNum==keyN) ? this : 0;
}


void
SurfPoint::setKeyN(const int Ky)
  /*!
    Sets the key and the sign 
    \param Ky :: key value (+/- is used for sign)
  */
{
  sign= (Ky<0) ? -1 : 1;
  keyN= sign*Ky;
  return;
}

void
SurfPoint::setKey(Surface* Spoint)
  /*!
    Sets the key pointter
    \param Spoint :: new key values
  */
{
  key=Spoint;
  return;
}

int
SurfPoint::simplify() 
  /*!
    Impossible to simplify a simple 
    rule leaf. Therefore returns 0
    \returns 0
  */
{
  return 0;
}

int
SurfPoint::isValid(const Geometry::Vec3D& Pt) const
  /*! 
    Determines if a point  is valid.  
    \param Pt :: Point to test
    \retval 1 :: Pt is the +ve side of the 
    surface or on the surface
    \retval 0 :: Pt is on the -ve side of the surface
  */
{
  if (key)
    return (key->side(Pt)*sign)>=0 ? 1 : 0;
  else
    return 0;
}

int
SurfPoint::isValid(const std::map<int,int>& MX) const
  /*! 
    Use MX to determine if the surface truth etc is 
    valid
    \param MX :: map of key + logical value XOR sign
    \returns MX.second if key found or 0
  */
{
  std::map<int,int>::const_iterator lx=MX.find(keyN);
  if (lx==MX.end())
    return 0;
  const int rtype=(lx->second) ? 1 : -1;
  return (rtype*sign)>=0 ? 1 : 0;
}

std::string
SurfPoint::display() const
  /*!
    Returns the signed surface number as
    a string.
    \returns string of the value
  */
{
  std::stringstream cx;
  cx<<sign*keyN;
  return cx.str();
}

std::string
SurfPoint::displayAddress() const
  /*!
    Returns the memory address as 
    a string.
    \returns memory address as string
  */
{
  std::stringstream cx;
  cx<<reinterpret_cast<long int>(this);
  return cx.str();
}

//----------------------------------------
//       COMPOBJ
//----------------------------------------
Kernel::Logger& CompObj::PLog(Kernel::Logger::get("CompObj"));

CompObj::CompObj() : Rule(),
  objN(0),key(0)
  /*!
    Constructor
  */
{}

CompObj::CompObj(const CompObj& A) : 
  Rule(A),
  objN(A.objN),key(A.key)
  /*!
    Standard copy constructor
    \param A :: CompObj to copy
   */
{}

CompObj&
CompObj::operator=(const CompObj& A)
  /*!
    Standard assignment operator
    \param A :: CompObj to copy
    \return *this
   */
{
  if (this!=&A)
    {
      Rule::operator=(A);
      objN=A.objN;
      key=A.key;
    }
  return *this;
}

CompObj::~CompObj()
  /*!
    Destructor
  */
{}


CompObj*
CompObj::clone() const
  /*!
    Clone of this
    \return new copy of this
  */
{
  return new CompObj(*this);
}

void
CompObj::setObjN(const int Ky)
  /*!
    Sets the object Number
    \param Ky :: key value 
  */
{
  objN= Ky;
  return;
}

void
CompObj::setLeaf(Rule* aR,const int)
  /*!
    Replaces a leaf with a rule.
    This REQUIRES that aR is of type SurfPoint
    \param aR :: new rule
    \param int :: Null side point
  */
{
  CompObj* newX = dynamic_cast<CompObj*>(aR);
  // Make a copy
  if (newX)
    *this = *newX;
  return;
}

void
CompObj::setLeaves(Rule* aR,Rule* oR)
  /*!
    Replaces a leaf with a rule.
    This REQUIRES that aR is of type CompObj
    \param aR :: new rule
    \param oR :: Null other rule
  */
{
  CompObj* newX = dynamic_cast<CompObj*>(aR);
  if (newX)
    *this = *newX;
  return;
}

Rule*
CompObj::findKey(const int i)
  /*!
    This is a complementary object and we dont
    search into CompObjs. If that is needed
    then the CompObj should be removed first
    \param i :: Null index key
    \return 0
  */
{
  return 0;
}

int
CompObj::findLeaf(const Rule* A) const
  /*!
    Check to see if this is a copy of a given Rule
    \param A :: Rule Ptr to find
    \retval 0 on success -ve on failuire
  */
{
  return (this==A) ? 0 : -1;
}

int
CompObj::isValid(const Geometry::Vec3D& Pt) const
  /*! 
    Determines if a point  is valid.  
    Checks to see if the point is valid in the object
    and returns ture if it is not valid.
    \param  Pt :: Point to test
    \retval not valid in the object 
    \retval true is no object is set
  */
{
  if (key)
    return (key->isValid(Pt)) ? 0 : 1;
  return 1;
}

int
CompObj::isValid(const std::map<int,int>& SMap) const
  /*! 
    Determines if a point  is valid.  
    \param  SMap :: map of surface number and true values
    \returns :: status
  */
{
  return (key) ? !(key->isValid(SMap)) : 1;
}

int
CompObj::simplify() 
  /*!
    Impossible to simplify a simple 
    rule leaf. Therefore returns 0
    \returns 0
  */
{
  return 0;
}

std::string
CompObj::display() const
  /*!
    Displays the object as \#number
    \returns string component
  */
{
  std::stringstream cx;
  cx<<"#"<<objN;
  return cx.str();
}

std::string
CompObj::displayAddress() const
  /*!
    Returns the memory address as 
    a string.
    \returns memory address as string
  */
{
  std::stringstream cx;
  cx<<reinterpret_cast<long int>(this);
  return cx.str();
}

// -----------------------------------------------
// BOOLVALUE
// -----------------------------------------------

Kernel::Logger& BoolValue::PLog(Kernel::Logger::get("BoolValue"));

BoolValue::BoolValue() : Rule()
  /*!
    Constructor
  */
{}

BoolValue::BoolValue(const BoolValue& A) : 
  Rule(A),
  status(A.status)
  /*!
    Copy Constructor 
    \param A :: BoolValue to copy
  */
{}

BoolValue&
BoolValue::operator=(const BoolValue& A)
  /*!
    Assignment operator 
    \param A :: BoolValue to copy
    \return *this
   */
{
  if (this!=&A)
    {
      Rule::operator=(A);
      status=A.status;
    }
  return *this;
}

BoolValue*
BoolValue::clone() const
  /*!
    Clone constructor
    \return new(*this)
   */
{
  return new BoolValue(*this);
}

BoolValue::~BoolValue()
  /*!
    Destructor
  */
{}

void
BoolValue::setLeaf(Rule* aR,const int)
  /*!
    Replaces a leaf with a rule.
    This REQUIRES that aR is of type SurfPoint
    \param aR :: new rule
    \param int :: Null side point
  */
{
  std::cerr<<"Calling BoolValue setLeaf"<<std::endl;
  BoolValue* newX = dynamic_cast<BoolValue*>(aR);
  if (newX)
    *this = *newX;
  return;
}

void
BoolValue::setLeaves(Rule* aR,Rule*)
  /*!
    Replaces a leaf with a rule.
    This REQUIRES that aR is of type SurfPoint
    \param aR :: new rule
    \param :: Null other rule
  */
{
  std::cerr<<"Calling BoolValue setLeaves"<<std::endl;
  BoolValue* newX = dynamic_cast<BoolValue*>(aR);
  if (newX)
    *this = *newX;
  return;
}

int
BoolValue::findLeaf(const Rule* A) const
{
  return (this==A) ? 0 : -1;
}

int
BoolValue::isValid(const Geometry::Vec3D&) const
  /*! 
    Determines if a point  is valid.  
    \param  :: Point to test
    \returns status
  */
{
  return status;
}

int
BoolValue::isValid(const std::map<int,int>&) const
  /*! 
    Determines if a point  is valid.  
    \param  :: map of surface number and true values
    \returns :: status
  */
{
  return status;
}

int
BoolValue::simplify()
  /*!
    Bool value is always in simplest form
    \return 0 
  */
{
  return 0;
}

std::string
BoolValue::display() const
  /*!
    \returns string of 
     - "true" "false" or "unknown"
  */

{
  switch(status) 
    {
    case 1:
      return " True ";
    case -1:
      return " False ";
    }
  return " Unkown ";
}


std::string
BoolValue::displayAddress() const
  /*!
    Returns the memory address as 
    a string.
    \returns string of Address
  */
{
  std::stringstream cx;
  cx<<reinterpret_cast<long int>(this);
  return cx.str();
}
  

//----------------------------------------
//       COMPGRP
//----------------------------------------
Kernel::Logger& CompGrp::PLog(Kernel::Logger::get("CompGrp"));

CompGrp::CompGrp() : 
  Rule(),A(0)
  /*!
    Constructor
  */
{}

CompGrp::CompGrp(Rule* Parent,Rule* Cx) :
  Rule(Parent),A(Cx)
  /*!
    Constructor to build parent and complent tree
    \param Parent :: Rule that is the parent to this
    \param Cx :: Complementary object below
  */
{
  if (Cx)
    Cx->setParent(this);
}

CompGrp::CompGrp(const CompGrp& Cother) : 
  Rule(Cother),A(0)
  /*!
    Standard copy constructor
    \param Cother :: CompGrp to copy
   */
{
  if (Cother.A)
    {
      A=Cother.A->clone();
      A->setParent(this);
    }
}

CompGrp&
CompGrp::operator=(const CompGrp& Cother)
  /*!
    Standard assignment operator
    \param Cother :: CompGrp to copy
    \return *this
   */
{
  if (this!=&Cother)
    {
      Rule::operator=(Cother);
      if (Cother.A)
        {
	  Rule* Xa=Cother.A->clone();
	  delete A;
	  A=Xa;
	  A->setParent(this);
	}
    }
  return *this;
}

CompGrp::~CompGrp()
  /*!
    Destructor
  */
{
  delete A;
}


CompGrp*
CompGrp::clone() const
  /*!
    Clone of this
    \return new copy of this
  */
{
  return new CompGrp(*this);
}

void
CompGrp::setLeaf(Rule* nR,const int side)
  /*!
    Replaces a leaf with a rule.
    No deletion is carried out
    \param nR :: new rule
    \param side :: side to use 
  */
{
  A=aR;
  if (A)
    A->setParent(this);
  return;
}

void
CompGrp::setLeaves(Rule* aR,Rule* oR)
  /*!
    Replaces a leaf with a rule.
    No deletion is carried out but sets the parents.
    \param aR :: new rule
    \param oR :: Null other rule
  */
{
  A=aR;
  if (A)
    A->setParent(this);
  return;
}

Rule*
CompGrp::findKey(const int i)
  /*!
    This is a complementary object and we dont
    search into CompGrps. If that is needed
    then the CompGrp should be removed first
    \param i :: Null index key
    \return 0
  */
{
  return 0;
}

int
CompGrp::findLeaf(const Rule* R) const
  /*!
    Check to see if this is a copy of a given Rule
    \param R :: Rule Ptr to find
    \retval 0 on success -ve on failuire
  */
{
  return (A==R) ? 0 : -1;
}

int
CompGrp::isValid(const Geometry::Vec3D& Pt) const
  /*! 
    Determines if a point  is valid.  
    Checks to see if the point is valid in the object
    and returns ture if it is not valid.
    Note the complementary reverse in the test.
    \param  Pt :: Point to test
    \retval not valid in the object 
    \retval true is no object is set
  */
{
  // Note:: if isValid is true then return 0:
  if (A)
    return (A->isValid(Pt)) ? 0 : 1;
  return 1;
}

int
CompGrp::isValid(const std::map<int,int>& SMap) const
  /*! 
    Determines if a point  is valid.  
    \param  SMap :: map of surface number and true values
    \returns :: status
  */
{
  // Note:: if isValid is true then return 0:
  if (A)
    return (A->isValid(SMap)) ? 0 : 1;
  return 1;
}

int
CompGrp::simplify() 
  /*!
    Impossible to simplify a simple 
    rule leaf. Therefore returns 0
    \returns 0
  */
{
  return 0;
}

std::string
CompGrp::display() const
  /*!
    Displays the object as \#number
    \returns string component
  */
{
  std::stringstream cx;
  if (A)
    cx<<"#( "<< A->display()<<" )";
  return cx.str();
}

std::string
CompGrp::displayAddress() const
  /*!
    Returns the memory address as 
    a string.
    \returns memory address as string
  */
{
  std::stringstream cx;
  cx<<"#( ["<<reinterpret_cast<long int>(this) <<"] ";
  if (A)
    cx<<A->displayAddress();
  else
    cx<<"0x0";
  cx<<" ) ";
  return cx.str();
}


}  // NAMESPACE Geometry



/// \Cond TEMPLATE

// template class DTriple<Mantid::Geometry::Rule*,int,Mantid::Geometry::Rule*>;

/// \Endcond TEMPLATE


}  // NAMESPACE Mantid
