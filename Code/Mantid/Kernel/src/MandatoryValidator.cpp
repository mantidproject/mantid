//------------------------------------------
// Includes
//------------------------------------------
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/System.h"
#include <cmath>

namespace Mantid
{
namespace Kernel
{

template<>
std::string MandatoryValidator<int>::checkValidity(const int& value) const
{
  if ( value == Mantid::EMPTY_INT() ) return "A value must be entered for this parameter";
  else return "";
}

template<>
std::string MandatoryValidator<double>::checkValidity(const double& value) const
{
  if( std::abs(value - Mantid::EMPTY_DBL()) < 1e-08 ) 
  {
    return "A value must be entered for this parameter";
  }
  else return "";
}


}
}
