#include "MantidKernel/RebinParamsValidator.h"

namespace Mantid
{
namespace Kernel
{

/** Check on the inputed bin boundaries and widths.
 *  @param value :: The parameter array to check
 *  @return A user level description of any problem or "" if there is no problem
 */
std::string RebinParamsValidator::checkValidity( const std::vector<double>& value ) const
{
  // array must not be empty
  if ( value.empty() ) return "Enter values for this property";
  // it must have an odd number of values (and be at least 3 elements long)
  if ( ( value.size()%2 == 0 ) || ( value.size() == 1 ) )
  {
    return "The number of bin boundaries must be even";
  }

  // bin widths must not be zero
  for(size_t i=1; i < value.size(); i+=2)
  {
    if (value[i] == 0.0)
    {
      return "Cannot have a zero bin width";
    }
  }  
  
  // bin boundary values must be in increasing order
  double previous = value[0];
  for(size_t i=2; i < value.size(); i+=2)
  {
    if ((value[i-1] < 0) && (previous <= 0))
    {
      return "Bin boundaries must be positive for logarithmic binning";
    }
    if (value[i] <= previous)
    {
      return "Bin boundary values must be given in order of increasing value";
    }
    else previous = value[i];
  }
  
  // All's OK if we get to here
  return "";
}

} // namespace Kernel
} // namespace Mantid
