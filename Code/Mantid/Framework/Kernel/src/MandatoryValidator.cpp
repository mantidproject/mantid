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

/** Checks if the integer it is passed equals the flag value 
 *  Mantid::EMPTY_DBL(), which implies that it wasn't set by the user
 *  @param value :: the value to test
 *  @return "A value must be entered for this parameter" if empty or ""
 */
template<>
std::string MandatoryValidator<int>::checkValidity(const int& value) const
{
  if ( value == Mantid::EMPTY_INT() ) return "A value must be entered for this parameter";
  else return "";
}

/** Checks if the double it is passed is within 10 parts per billon of flag
 *  value Mantid::EMPTY_DBL(), which implies that it wasn't set by the user
 *  @param value :: the value to test
 *  @return "A value must be entered for this parameter" if empty or ""
 */
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
