#include "MantidSINQ/PoldiUtilities/Poldi2DFunction.h"

namespace Mantid
{
namespace Poldi
{
using namespace API;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  Poldi2DFunction::Poldi2DFunction() :
      IFunction1DSpectrum(),
      CompositeFunction()
  {
  }

  void Poldi2DFunction::function(const FunctionDomain &domain, FunctionValues &values) const
  {
      CompositeFunction::function(domain, values);
  }

  void Poldi2DFunction::functionDeriv(const FunctionDomain &domain, Jacobian &jacobian)
  {
      CompositeFunction::functionDeriv(domain, jacobian);
  }

  void Poldi2DFunction::function1DSpectrum(const API::FunctionDomain1DSpectrum &domain, API::FunctionValues &values) const
  {
    UNUSED_ARG(domain);
    UNUSED_ARG(values);
  }
  


} // namespace SINQ
} // namespace Mantid
