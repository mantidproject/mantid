// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction1D.hxx"
#include "MantidAPI/IFunctionWithLocation.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/TextAxis.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitFactory.h"

#include <sstream>

namespace Mantid::API {
using namespace Geometry;

/// init logger
Kernel::Logger IFunction1D::g_log("IFunction1D");

void IFunction1D::function(const FunctionDomain &domain, FunctionValues &values) const {
  auto histoDomain = dynamic_cast<const FunctionDomain1DHistogram *>(&domain);
  if (histoDomain) {
    histogram1D(values.getPointerToCalculated(0), histoDomain->leftBoundary(), histoDomain->getPointerAt(0),
                histoDomain->size());
    return;
  }
  const auto *d1d = dynamic_cast<const FunctionDomain1D *>(&domain);
  if (!d1d) {
    throw std::invalid_argument("Unexpected domain in IFunction1D");
  }
  function1D(values.getPointerToCalculated(0), d1d->getPointerAt(0), d1d->size());
}

void IFunction1D::functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) {
  auto histoDomain = dynamic_cast<const FunctionDomain1DHistogram *>(&domain);
  if (histoDomain) {
    histogramDerivative1D(&jacobian, histoDomain->leftBoundary(), histoDomain->getPointerAt(0), histoDomain->size());
    return;
  }
  const auto *d1d = dynamic_cast<const FunctionDomain1D *>(&domain);
  if (!d1d) {
    throw std::invalid_argument("Unexpected domain in IFunction1D");
  }
  functionDeriv1D(&jacobian, d1d->getPointerAt(0), d1d->size());
}

void IFunction1D::derivative(const FunctionDomain &domain, FunctionValues &values, const size_t order) const {
  const auto *d1d = dynamic_cast<const FunctionDomain1D *>(&domain);
  if (!d1d) {
    throw std::invalid_argument("Unexpected domain in IFunction1D");
  }

  derivative1D(values.getPointerToCalculated(0), d1d->getPointerAt(0), d1d->size(), order);
}

void IFunction1D::derivative1D(double *out, const double *xValues, size_t nData, const size_t order) const {
  UNUSED_ARG(out);
  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);
  UNUSED_ARG(order);
  throw Kernel::Exception::NotImplementedError("Derivative is not implemented for this function.");
}

void IFunction1D::functionDeriv1D(Jacobian *jacobian, const double *xValues, const size_t nData) {
  auto evalMethod = [this](double *out, const double *xValues, const size_t nData) {
    this->function1D(out, xValues, nData);
  };
  this->calcNumericalDerivative1D(jacobian, std::move(evalMethod), xValues, nData);
}

/// Calculate histogram data for the given bin boundaries.
/// @param out :: Output bin values (size == nBins) - integrals of the function
///    inside each bin.
/// @param left :: The left-most bin boundary.
/// @param right :: A pointer to an array of successive right bin boundaries
/// (size = nBins).
/// @param nBins :: Number of bins.
void IFunction1D::histogram1D(double *out, double left, const double *right, const size_t nBins) const {
  UNUSED_ARG(out);
  UNUSED_ARG(left);
  UNUSED_ARG(right);
  UNUSED_ARG(nBins);
  throw Kernel::Exception::NotImplementedError("Integration is not implemented for this function.");
}

/// Derivatives of the histogram.
/// @param jacobian :: The output Jacobian.
/// @param left :: The left-most bin boundary.
/// @param right :: A pointer to an array of successive right bin boundaries
/// (size = nBins).
/// @param nBins :: Number of bins.
void IFunction1D::histogramDerivative1D(Jacobian *jacobian, double left, const double *right,
                                        const size_t nBins) const {
  UNUSED_ARG(jacobian);
  UNUSED_ARG(left);
  UNUSED_ARG(right);
  UNUSED_ARG(nBins);
  throw Kernel::Exception::NotImplementedError("Integration is not implemented for this function.");
}

} // namespace Mantid::API
