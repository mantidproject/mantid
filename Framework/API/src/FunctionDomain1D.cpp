//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionDomain1D.h"

namespace Mantid {
namespace API {

/// The constructor
FunctionDomain1D::FunctionDomain1D(const double *x, size_t n)
    : m_data(x), m_n(n), m_peakRadius(0) {}

/// Convert to a vector
std::vector<double> FunctionDomain1D::toVector() const {
  std::vector<double> res;
  if (m_n > 0) {
    res.assign(m_data, m_data + m_n);
  }
  return res;
}

/**
 * Set a peak redius to pass to peak functions.
 * @param radius :: New radius value.
 */
void FunctionDomain1D::setPeakRadius(int radius) { m_peakRadius = radius; }

/**
 * Get the peak radius.
 */
int FunctionDomain1D::getPeakRadius() const { return m_peakRadius; }

/**
 * Create a domain from a vector.
 * @param xvalues :: Vector with function arguments to be copied from.
 */
FunctionDomain1DVector::FunctionDomain1DVector(
    const std::vector<double> &xvalues)
    : FunctionDomain1D(nullptr, 0) {
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
    : FunctionDomain1D(nullptr, 0) {
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
    : FunctionDomain1D(nullptr, 0) {
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
    : FunctionDomain1D(nullptr, 0) {
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
    : FunctionDomain1D(nullptr, 0) {
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

/// Constructor.
/// @param bins :: A vector with bin boundaries.
FunctionDomain1DHistogram::FunctionDomain1DHistogram(
    const std::vector<double> &bins)
    : FunctionDomain1DHistogram(bins.begin(), bins.end()) {}

/**
 * Create a domain from a part of a vector.
 * @param from :: Iterator to start copying values from.
 * @param to :: Iterator to the end of the data.
 */
FunctionDomain1DHistogram::FunctionDomain1DHistogram(
    std::vector<double>::const_iterator from,
    std::vector<double>::const_iterator to)
    : FunctionDomain1D(nullptr, 0), m_bins(from, to) {
  if (m_bins.size() < 2) {
    throw std::runtime_error("Cannot initialize FunctionDomain1DHistogram with "
                             "less than 2 bin boundaries.");
  }
  resetData(&m_bins[1], m_bins.size() - 1);
}

/// Get the leftmost boundary
double FunctionDomain1DHistogram::leftBoundary() const {
  return m_bins.front();
}

} // namespace API
} // namespace Mantid
