#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid {
namespace DataObjects {

void Histogram1D::clearData() {
  MantidVec &yValues = this->dataY();
  std::fill(yValues.begin(), yValues.end(), 0.0);
  MantidVec &eValues = this->dataE();
  std::fill(eValues.begin(), eValues.end(), 0.0);
}

/// Sets the x data.
/// @param X :: vector of X data
void Histogram1D::setX(const Kernel::cow_ptr<HistogramData::HistogramX> &X) {
  m_histogram.setX(X);
}

/// Returns the x data
MantidVec &Histogram1D::dataX() { return m_histogram.dataX(); }

/// Returns the x data const
const MantidVec &Histogram1D::dataX() const { return m_histogram.dataX(); }

/// Returns the x data const
const MantidVec &Histogram1D::readX() const { return m_histogram.readX(); }

/// Returns a pointer to the x data
Kernel::cow_ptr<HistogramData::HistogramX> Histogram1D::ptrX() const {
  return m_histogram.ptrX();
}

} // namespace DataObjects
} // namespace Mantid
