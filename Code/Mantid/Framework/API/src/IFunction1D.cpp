//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/IFunctionWithLocation.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TextAxis.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/muParser_Silent.h"
#include <boost/lexical_cast.hpp>

#include <sstream>
#include <iostream> 

namespace Mantid
{
namespace API
{
  using namespace Geometry;
  
  Kernel::Logger& IFunction1D::g_log = Kernel::Logger::get("IFunction1D");

void IFunction1D::function(const FunctionDomain& domain,FunctionValues& values)const
{
  const FunctionDomain1D* d1d = dynamic_cast<const FunctionDomain1D*>(&domain);
  if (!d1d)
  {
    throw std::invalid_argument("Unexpected domain in IFunction1D");
  }
  function1D(values.getPointerToCalculated(0),d1d->getPointerAt(0),d1d->size());
}

void IFunction1D::functionDeriv(const FunctionDomain& domain, Jacobian& jacobian)
{
  const FunctionDomain1D* d1d = dynamic_cast<const FunctionDomain1D*>(&domain);
  if (!d1d)
  {
    throw std::invalid_argument("Unexpected domain in IFunction1D");
  }
  functionDeriv1D(&jacobian,d1d->getPointerAt(0),d1d->size());
}

/** Base class implementation of derivative IFunction1D throws error. This is to check if such a function is provided
    by derivative class. In the derived classes this method must return the derivatives of the resuduals function
    (defined in void Fit1D::function(const double*, double*, const double*, const double*, const double*, const int&))
    with respect to the fit parameters. If this method is not reimplemented the derivative free simplex minimization
    algorithm is used.
 */
void IFunction1D::functionDeriv1D(Jacobian* jacobian, const double* xValues, const size_t nData)
{
  //throw Kernel::Exception::NotImplementedError("No derivative IFunction1D provided");
  FunctionDomain1DView domain(xValues,nData);
  this->calNumericalDeriv(domain,*jacobian);
}

} // namespace API
} // namespace Mantid
