#include "MantidKernel/RebinParamsValidator.h"

namespace Mantid
{
namespace Kernel
{

/** Check on the inputed bin boundaries and widths.
 *  @param value The parameter array to check
 *  @return A user level description of any problem or "" if there is no problem
 */
std::string RebinParamsValidator::checkValidity( const std::vector<double>& value ) const
{
  if ( value.empty() ) return "Enter values for this property";
  if ( ( value.size()%2 == 0 ) || ( value.size() == 1 ) )
  {
    return "The number of bin boundaries must be even";
  }

  double previous = value[0];
  for(size_t i=2; i < value.size(); i+=2)
  {
    if (value[i] <= previous)
	{
	  return "Bin boundary values must be given in order of increasing value";
	}
	else previous = value[i];
  }
  return "";
}

} // namespace Kernel
} // namespace Mantid
