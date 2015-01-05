//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionDomain1D.h"
#include <iostream>

namespace Mantid {
namespace API {

/**
  * Create a domain from a vector.
  * @param xvalues :: Vector with function arguments to be copied from.
  */
FunctionDomain1DVector::FunctionDomain1DVector(
    const std::vector<double> &xvalues)
    : FunctionDomain1D(NULL, 0) {
  if (xvalues.empty()) {
    throw std::invalid_argument("FunctionDomain1D cannot have zero size.");
  }
  m_X.assign(xvalues.begin(), xvalues.end());
  resetData(&m_X[0], m_X.size());
}

/**
  * Create a domain from a part of a vector.
  * @param from :: Iterator to start copying values from.
  * @param to :: Iterator to the end of the data.
  */
FunctionDomain1DVector::FunctionDomain1DVector(
    std::vector<double>::const_iterator from,
    std::vector<double>::const_iterator to)
    : FunctionDomain1D(NULL, 0) {
  if (from == to) {
    throw std::invalid_argument("FunctionDomain1D cannot have zero size.");
  }
  m_X.assign(from, to);
  resetData(&m_X[0], m_X.size());
}

/**
 * Create a domain with regular values in an interval. The step in values is
 * (endX - startX) / (n - 1).
 * If n == 1 creates a single value of (startX + endX) / 2.
 * @param startX :: The start of an interval.
 * @param endX :: The end of an interval.
 * @param n :: Number of values to create.
 */
FunctionDomain1DVector::FunctionDomain1DVector(const double startX,
                                               const double endX,
                                               const size_t n)
    : FunctionDomain1D(NULL, 0) {
  if (n == 0) {
    throw std::invalid_argument("FunctionDomain1D cannot have zero size.");
  }
  m_X.resize(n);
  if (n == 1) {
    m_X[0] = (startX + endX) / 2;
  } else {
    const double dx = (endX - startX) / double(n - 1);
    for (size_t i = 0; i < n; ++i) {
      m_X[i] = startX + dx * double(i);
    }
  }
  resetData(&m_X[0], m_X.size());
}

/**
 * Create a domain with a single value.
 * @param x :: The argument value.
 */
FunctionDomain1DVector::FunctionDomain1DVector(const double x)
    : FunctionDomain1D(NULL, 0) {
  m_X.resize(1);
  m_X[0] = x;
  resetData(&m_X[0], m_X.size());
}

/**
 * Copy constructor.
 * @param right :: The other domain.
 */
FunctionDomain1DVector::FunctionDomain1DVector(
    const FunctionDomain1DVector &right)
    : FunctionDomain1D(NULL, 0) {
  *this = right;
}

/**
 * Copy assignment operator.
 * @param right :: The other domain.
 */
FunctionDomain1DVector &FunctionDomain1DVector::
operator=(const FunctionDomain1DVector &right) {
  if (right.m_X.empty()) {
    throw std::invalid_argument("FunctionDomain1D cannot have zero size.");
  }
  m_X.assign(right.m_X.begin(), right.m_X.end());
  resetData(&m_X[0], m_X.size());
  return *this;
}

/**
  * Create a domain from a vector.
  * @param wi :: The workspace index of a spectrum the xvalues come from.
  * @param xvalues :: Vector with function arguments to be copied from.
  */
FunctionDomain1DSpectrum::FunctionDomain1DSpectrum(
    size_t wi, const std::vector<double> &xvalues)
    : FunctionDomain1DVector(xvalues), m_workspaceIndex(wi) {}

/**
  * Create a domain from a part of a vector.
  * @param wi :: The workspace index of a spectrum the x-values come from.
  * @param from :: Iterator to start copying values from.
  * @param to :: Iterator to the end of the data.
  */
FunctionDomain1DSpectrum::FunctionDomain1DSpectrum(
    size_t wi, std::vector<double>::const_iterator from,
    std::vector<double>::const_iterator to)
    : FunctionDomain1DVector(from, to), m_workspaceIndex(wi) {}

} // namespace API
} // namespace Mantid
