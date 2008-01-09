#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>
#include <gsl/gsl_poly.h>

#include "AuxException.h"
#include "PolyFunction.h"

namespace Mantid
{

namespace mathLevel
{

std::ostream& 
operator<<(std::ostream& OX,const PolyFunction& A)
  /*!
    External Friend :: outputs point to a stream 
    \param OX :: output stream
    \param A :: PolyFunction to write
    \returns The output stream (OX)
  */
{
  (&A)->write(OX);
  return OX;
}

PolyFunction::PolyFunction() : 
  Eaccuracy(1e-6)
  /*!
    Constructor 
  */
{}

PolyFunction::PolyFunction(const double E) : 
  Eaccuracy(fabs(E))
  /*!
    Constructor 
    \param E :: Accuracy
  */
{}

PolyFunction::PolyFunction(const PolyFunction& A)  :
  Eaccuracy(A.Eaccuracy)
  /*! 
    Copy Constructor
    \param A :: PolyFunction to copy
   */
{ }

PolyFunction& 
PolyFunction::operator=(const PolyFunction& A)
  /*! 
    Assignment operator
    \param A :: PolyFunction to copy
    \return *this
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
  /*!
    Basic write command
    \param OX :: output stream
  */
{
  return;
}  


}  // NAMESPACE  mathLevel

}  // NAMESPACE Mantid


