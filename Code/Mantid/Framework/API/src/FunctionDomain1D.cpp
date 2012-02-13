//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionDomain1D.h"
#include <iostream> 

namespace Mantid
{
namespace API
{

/**
  * Create a domain form a vector.
  * @param xvalues :: Points
  */
FunctionDomain1D::FunctionDomain1D(const std::vector<double>& xvalues)
{
  if (xvalues.empty())
  {
    throw std::invalid_argument("FunctionDomain1D cannot have zero size.");
  }
  m_X.assign(xvalues.begin(),xvalues.end());
}


} // namespace API
} // namespace Mantid
