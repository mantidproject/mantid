#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace DataObjects {

/// Construct empty
Histogram1D::Histogram1D(HistogramData::Histogram::XMode xmode,
                         HistogramData::Histogram::YMode ymode)
    : API::ISpectrum(), m_histogram(xmode, ymode) {
  if (ymode == HistogramData::Histogram::YMode::Counts) {
    m_histogram.setCounts(0);
    m_histogram.setCountStandardDeviations(0);
  } else if (ymode == HistogramData::Histogram::YMode::Frequencies) {
    m_histogram.setFrequencies(0);
    m_histogram.setFrequencyStandardDeviations(0);
  } else {
    throw std::logic_error("Histogram1D: YMode must be Counts or Frequencies");
  }
}

/// Construct from ISpectrum.
Histogram1D::Histogram1D(const ISpectrum &other)
    : ISpectrum(other), m_histogram(other.histogram()) {}

/// Assignment from ISpectrum.
Histogram1D &Histogram1D::operator=(const ISpectrum &rhs) {
  ISpectrum::operator=(rhs);
  m_histogram = rhs.histogram();
  return *this;
}

void Histogram1D::clearData() {
  MantidVec &yValues = this->dataY();
  std::fill(yValues.begin(), yValues.end(), 0.0);
  MantidVec &eValues = this->dataE();
  std::fill(eValues.begin(), eValues.end(), 0.0);
}

/// Deprecated, use setSharedX() instead. Sets the x data.
/// @param X :: vector of X data
void Histogram1D::setX(const Kernel::cow_ptr<HistogramData::HistogramX> &X) {
  m_histogram.setX(X);
}

/// Deprecated, use mutableX() instead. Returns the x data
MantidVec &Histogram1D::dataX() { return m_histogram.dataX(); }

/// Deprecated, use x() instead. Returns the x data const
const MantidVec &Histogram1D::dataX() const { return m_histogram.dataX(); }

/// Deprecated, use x() instead. Returns the x data const
const MantidVec &Histogram1D::readX() const { return m_histogram.readX(); }

/// Deprecated, use sharedX() instead. Returns a pointer to the x data
Kernel::cow_ptr<HistogramData::HistogramX> Histogram1D::ptrX() const {
  return m_histogram.ptrX();
}

/// Deprecated, use mutableDx() instead.
MantidVec &Histogram1D::dataDx() { return m_histogram.dataDx(); }
/// Deprecated, use dx() instead.
const MantidVec &Histogram1D::dataDx() const { return m_histogram.dataDx(); }
/// Deprecated, use dx() instead.
const MantidVec &Histogram1D::readDx() const { return m_histogram.readDx(); }

/**
 * Makes sure a histogram has valid Y and E data.
 * @param histogram A histogram to check.
 * @throw std::invalid_argument if Y or E data is NULL.
 */
void Histogram1D::checkAndSanitizeHistogram(
    HistogramData::Histogram &histogram) {
  if (!histogram.sharedY()) {
    throw std::invalid_argument(
        "Histogram1D: invalid input: Y data set to nullptr");
  }
  if (!histogram.sharedE()) {
    throw std::invalid_argument(
        "Histogram1D: invalid input: E data set to nullptr");
  }
}

} // namespace DataObjects
} // namespace Mantid
