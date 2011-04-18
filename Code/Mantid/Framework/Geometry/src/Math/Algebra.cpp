#include <fstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <string>
#include <sstream>
#include <algorithm>
#include <functional>
#include <iterator>

#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"
#include "MantidGeometry/Math/MapSupport.h"
#include "MantidGeometry/Math/Algebra.h"


namespace Mantid
{

namespace Geometry
{

std::ostream&
operator<<(std::ostream& OX,const Algebra& A)
  /**
    Write to standard stream
    @param OX :: Output stream
    @param A :: Algebra to write
    @return stream representation
   */
{
  OX<<A.display();
  return OX;
}

Kernel::Logger& Algebra::PLog(Kernel::Logger::get("Algebra"));

Algebra::Algebra() :
  F(0)
  /**
    Constructor
  */
{}

Algebra::Algebra(const Algebra& A) :
  SurfMap(A.SurfMap),F(A.F)
  /**
    Copy Constructor 
    @param A :: Algebra to copy
  */
{}

Algebra&
Algebra::operator=(const Algebra& A) 
  /**
    Assignment operator
    @param A :: object to copy
    @return *this
  */
{
  if (this!=&A)
    {
      SurfMap=A.SurfMap;
      F=A.F;
    }
  return *this;
}

/// Destructor
Algebra::~Algebra()
{ }


bool
Algebra::operator==(const Algebra& A) const
  /**
    Equality operator
    @param A :: object to compary
    @return this==A
  */
{
  if (this==&A)
    return true;
  return (F==A.F);
}

bool
Algebra::operator!=(const Algebra& A) const
  /**
    Inequality operator
    @param A :: object to compary
    @return this!=A (via ==)
  */
{
  return (F!=A.F);
}

Algebra&
Algebra::operator+=(const Algebra& M)
  /**
    Adds this by M algebrically 
    @param M :: Algebric object to add
    @return *this
  */
{
  F+=M.F;
  return *this;
}

Algebra&
Algebra::operator*=(const Algebra& M)
  /**
    Multiplies this by M algebrically 
    @param M :: Algebric object to multiply by
    @return *this
  */
{
  F*=M.F;
  return *this;
}

Algebra
Algebra::operator+(const Algebra& M) const
  /**
    Addition operator (or construction)
    @param M :: Algebra to add
    @return this+M
  */
{
  Algebra T(*this);
  T+=M;
  return T;
}

Algebra
Algebra::operator*(const Algebra& M) const
  /**
    Addition operator (and construction)
    @param M :: Algebra to multiply (and)
    @return this+M
  */
{
  Algebra T(*this);
  T*=M;
  return T;
}

void
Algebra::Complement()
  /**
    Takes the complement of the algebric
    function.
  */
{
  F.complement();
}

std::pair<Algebra,Algebra>
Algebra::algDiv(const Algebra& D) const
  /**
    Divide by D algebrically
    @param D :: Divisor
    @return Quotian + Remainder
   */
{
  Algebra Q;
  Algebra R;
  Acomp Tf=F;
  //std::cerr<<"AlgDiv:"<<std::endl;
  std::pair<Acomp,Acomp> QR=Tf.algDiv(D.F);
  if (!QR.first.isNull() && 
      !QR.second.isNull())
    {
      Q.setFunction(QR.first);
      R.setFunction(QR.second);
    }
  return std::pair<Algebra,Algebra>(Q,R);
}


std::string
Algebra::writeMCNPX() const
  /**
    Writes out the string in terms
    of surface numbers for MCNPX.
    Note the ugly use of valEqual to find the cell
    since the SrufMap is the wrong way round.
    This also has the problem that Algebra uses
    intersection as master but MCNPX uses union 
    @return string representation of MCNPX
  */
{
  std::string Out=F.display();
  const int lenOut = static_cast<const int>(Out.length());
  Out+=" ";      // Guard string
  std::ostringstream cx;
  for(int i=0;i<lenOut;i++)
    {
      if (islower(Out[i]) || isupper(Out[i]))
        {
	  std::map<int,std::string>::const_iterator vc=
	    find_if(SurfMap.begin(),SurfMap.end(),
		     MapSupport::valEqual<int,std::string>(std::string(1,Out[i])));
	  if (vc==SurfMap.end())
            {
	      std::cout<<"SurfMap size == "<<SurfMap.size()<<std::endl;
	      for_each(SurfMap.begin(),SurfMap.end(),
		       MapSupport::mapWrite<int,std::string>());
        throw Kernel::Exception::NotFoundError("Algebra::writeMCNPX",std::string(1,Out[i]));
	    }
	  if (Out[i+1]=='\'')
	    cx<<" -"<<vc->first;
	  else
	    cx<<" "<<vc->first;
	}
      else if (Out[i]=='+')
        {
	  cx<<" :";
	}
      else       // brackets are constant
        {
	  cx<<" "<<Out[i];
	}
    }
  return cx.str();
}

std::ostream&
Algebra::write(std::ostream& Out) const
  /**
    Output function
    @param Out :: Ostream to write out
    @return Out
  */
{
  Out<<"F == "<<F.display()<<std::endl;
  //  Out<<F.displayDepth(0)<<std::endl;
  return Out;
}


std::string Algebra::display() const
{
  return F.display();

}

int
Algebra::setFunctionObjStr(const std::string& A)
  /**
    Fill the algebra (AComp) with an object given an 
    MCNPX String. 
    The string type is of 3 : 5 : 6 -4 #( xx  ) 
    - where #( ) surroud the string
    @param A :: string to process (stripped of id,density etc)
    @retval -1 ::  Failure
    @retval 0 ::  Success
  */
{
  // get first item
  std::ostringstream cx;
  std::string nLiteral="a";
   
  int ipt(0);    // start of component
  int bigFlag(0);  // Literals getting big
  while (ipt<static_cast<int>(A.length()))
    {

      if (A[ipt]=='(' || A[ipt]==')')
        {
	  cx<<A[ipt];
	  ipt++;
	}
      else if (A[ipt]=='-' || isdigit(A[ipt]) )
        {
	  int N;
	  int neg(0);
	  int nCount=Mantid::Kernel::Strings::convPartNum(A.substr(ipt,std::string::npos),N);
	  if (nCount)
	    {
	      if (N<0)
	        {
		  N*=-1;
		  neg=1;
		}
	      std::map<int,std::string>::iterator mc=SurfMap.find(N);
	      if (mc==SurfMap.end())
	        {
		  if (!bigFlag)
		    {
		      SurfMap[N]=nLiteral;
		      cx<<nLiteral;
		      nLiteral[0]= (nLiteral[0]=='z') ? 'A' : static_cast<int>(nLiteral[0])+1;
		      bigFlag=(nLiteral[0]=='Z') ? 1 : 0;
		    }
		  else
		    {
		      std::ostringstream lcx;
		      lcx<<"%"<<bigFlag;
		      SurfMap[N]=lcx.str();
		      cx<<lcx.str();
		      bigFlag++;
		    }
		}
	      else
	        {
		  cx<<mc->second;
		}
	      // Add negation note:
	      if (neg)
		cx<<"\'";
	      // Add to the number
	      ipt+=nCount;
	    }
	  else
	    {
	      std::cout<<"Algebra::setFunction: ncount==0"<<std::endl;
	      exit(1);
	    }
	}
      else if (A[ipt]==':')
        {
	  cx<<"+";
	  ipt++;
	}
      else if (A[ipt]=='#')
        {
	  cx<<"#";
	  ipt++;
	}
      else         // Space
	ipt++;
    }
  setFunction(cx.str());
  return 0;
}

int
Algebra::setFunction(const std::string& A)
  /**
    Set the function using a basic string (abc etc)
    @param A :: String to use for the function
    @retval 1 Error
    @retval 0 on success
  */
{
  // Get copy
  std::string Ln=A;
  // Strip spaces.
  std::string::size_type pos;
  while((pos=Ln.find(' '))!=std::string::npos)
    Ln.erase(pos,1);
  try
    {
      F.setString(Ln);
    }
  catch (...)
    {
      std::cerr<<"Algebra String Error"<<A<<std::endl;
      return 1;
    }
  return 0;
}

int
Algebra::setFunction(const Acomp& A)
  /**
    Set the function using a toplevel
    Acomp.
    @param A :: Acomp to be copied to F.
    @return 0 on success
  */
{
  F=A;
  return 0;
}

int
Algebra::countLiterals() const
  /**
    Count the number of different literals
    in the algebraic function
    Does this by generating the map of literals
    @return number of literals found
  */
{
  std::map<int,int> Lit;
  F.getLiterals(Lit);
  return static_cast<int>(Lit.size());
}

int
Algebra::logicalEqual(const Algebra& A) const
  /**
    Calculate if two functions are logically
    equivilent (exhaustive search)
    @param A :: Algrebra to testg
    @return True/False
   */  
{
  return F.logicalEqual(A.F);
}

} // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid
