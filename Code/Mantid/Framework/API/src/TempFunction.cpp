//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidAPI/TempFunction.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"

#include <sstream>
#include <iostream> 

namespace Mantid
{
namespace API
{
  
  TempFunction::TempFunction(IFitFunction* function):
  IFunction(),
  m_function(function)
  {}

} // namespace API
} // namespace Mantid
