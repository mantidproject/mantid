//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"

#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace API
{

/**
 * Do minimization of the set function.
 * @param maxIterations :: Maximum number of iterations.
 * @return :: true if successful, false otherwise. Call getError() to see the error message string.
 */
bool IFuncMinimizer::minimize(size_t maxIterations) 
{
  m_errorString = ""; // iterate() may modify it
  size_t iter = 0;
  bool success = false;
  do
  {
    if ( !iterate(iter) )
    {
      success = m_errorString.empty() || m_errorString == "success";
      m_errorString = "success";
      break;
    }
    iter++;
  }
  while (iter < maxIterations);

  if (iter >= maxIterations)
  {
    success = false;
    if ( !m_errorString.empty() )
    {
      m_errorString += '\n';
    }
    m_errorString += "Failed to converge after " + boost::lexical_cast<std::string>(maxIterations) + " iterations.";
  }

  return success;

}

} // namespace API
} // namespace Mantid
