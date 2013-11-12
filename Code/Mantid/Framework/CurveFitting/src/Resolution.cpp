//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Resolution.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Resolution)

Resolution::Resolution():TabulatedFunction()
{
    tie("Scaling","1.0"); // fix the scaling parameter
}


} // namespace CurveFitting
} // namespace Mantid
