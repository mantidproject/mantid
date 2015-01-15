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

namespace Mantid {
namespace API {
using namespace Geometry;

/// init logger
Kernel::Logger IFunction1D::g_log("IFunction1D");

void IFunction1D::function(const FunctionDomain &domain,
                           FunctionValues &values) const {
  const FunctionDomain1D *d1d = dynamic_cast<const FunctionDomain1D *>(&domain);
  if (!d1d) {
    throw std::invalid_argument("Unexpected domain in IFunction1D");
  }
  function1D(values.getPointerToCalculated(0), d1d->getPointerAt(0),
             d1d->size());
}

void IFunction1D::functionDeriv(const FunctionDomain &domain,
                                Jacobian &jacobian) {
  const FunctionDomain1D *d1d = dynamic_cast<const FunctionDomain1D *>(&domain);
  if (!d1d) {
    throw std::invalid_argument("Unexpected domain in IFunction1D");
  }
  functionDeriv1D(&jacobian, d1d->getPointerAt(0), d1d->size());
}

void IFunction1D::derivative(const FunctionDomain &domain,
                             FunctionValues &values, const size_t order) const {
  const FunctionDomain1D *d1d = dynamic_cast<const FunctionDomain1D *>(&domain);
  if (!d1d) {
    throw std::invalid_argument("Unexpected domain in IFunction1D");
  }

  derivative1D(values.getPointerToCalculated(0), d1d->getPointerAt(0),
               d1d->size(), order);
}

void IFunction1D::derivative1D(double *out, const double *xValues, size_t nData,
                               const size_t order) const {
  UNUSED_ARG(out);
  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);
  UNUSED_ARG(order);
  throw Kernel::Exception::NotImplementedError(
      "Derivative is not implemented for this function.");
}

void IFunction1D::functionDeriv1D(Jacobian *jacobian, const double *xValues,
                                  const size_t nData) {
  FunctionDomain1DView domain(xValues, nData);
  this->calNumericalDeriv(domain, *jacobian);
}

} // namespace API
} // namespace Mantid
