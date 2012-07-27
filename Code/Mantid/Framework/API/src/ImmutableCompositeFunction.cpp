//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/ImmutableCompositeFunction.h"

namespace Mantid
{
namespace API
{

using std::size_t;

  /**
   * Overridden method creates an initialization string which makes it look like a 
   * siple function.
   */
  std::string ImmutableCompositeFunction::asString()const
  {
    return "";
  }

} // namespace API
} // namespace Mantid
