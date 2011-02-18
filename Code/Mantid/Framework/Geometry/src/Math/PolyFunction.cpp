#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>
#include <gsl/gsl_poly.h>

#include "MantidKernel/Strings.h"
#include "MantidGeometry/Math/PolyFunction.h"

namespace Mantid
{
namespace mathLevel
{

std::ostream& 
operator<<(std::ostream& OX,const PolyFunction& A)
  /**
    External Friend :: outputs point to a stream 
    @param OX :: output stream
    @param A :: PolyFunction to write
    @return The output stream (OX)
  */
{
  (&A)->write(OX);
  return OX;
}

PolyFunction::PolyFunction() : 
  Eaccuracy(1e-6)
  /**
    Constructor 
  */
{}

PolyFunction::PolyFunction(const double E) : 
  Eaccuracy(fabs(E))
  /**
    Constructor 
    @param E :: Accuracy
  */
{}

PolyFunction::PolyFunction(const PolyFunction& A)  :
  Eaccuracy(A.Eaccuracy)
  /** 
    Copy Constructor
    @param A :: PolyFunction to copy
   */
{ }

PolyFunction& 
PolyFunction::operator=(const PolyFunction& A)
  /** 
    Assignment operator
    @param A :: PolyFunction to copy
    @return *this
   */
{
  if (this!=&A)
    {
      Eaccuracy=A.Eaccuracy;
    }
  return *this;
}

PolyFunction::~PolyFunction()
  /// Destructor
{}


void
PolyFunction::write(std::ostream& OX) const
  /**
    Basic write command
    @param OX :: output stream
  */
{
  (void) OX; //Avoid compiler warning
  return;
}  

int
PolyFunction::getMaxSize(const std::string& CLine,const char V)
  /**
    Finds the maximum power in the string
    of the variable type
    @param CLine :: Line to calcuate V^x componenets
    @param V :: Variable letter.
    @return Max Power 
  */
{
  int maxPower(0);
  std::string::size_type pos(0);
  pos=CLine.find(V,pos);
  const unsigned int L=CLine.length()-2;
  while(pos!=std::string::npos)
    {
      if (pos!=L && CLine[pos+1]=='^') 
        {
	  int pV;
	  if (Mantid::Kernel::Strings::convPartNum(CLine.substr(pos+2),pV) && pV>maxPower)
	    maxPower=pV;
	}
      else if (!maxPower)      // case of +y+... etc
	maxPower=1;
	
      pos=CLine.find(V,pos+1);
    }
  return maxPower;
}

}  // NAMESPACE  mathLevel


}  // NAMESPACE Mantid


