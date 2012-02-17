//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IFuncMinimizer.h"

namespace Mantid
{
namespace CurveFitting
{

/**
 * Do minimization of the set function.
 * @return :: true if successful, false otherwise. Call getError() to see the error message string.
 */
bool IFuncMinimizer::minimize() 
{
  m_errorString = "";
  size_t iter = 0;
  bool success = false;
  do
  {
    iter++;
    if ( !iterate() )
    {
      success = true;
      break;
    }
  }
  while (iter < 100); //! <---------------

  return success;

}

} // namespace CurveFitting
} // namespace Mantid
