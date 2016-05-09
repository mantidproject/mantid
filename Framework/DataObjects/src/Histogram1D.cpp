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

/// Sets the dx data.
/// @param Dx :: vector of Dx data
void Histogram1D::setDx(const Kernel::cow_ptr<HistogramData::HistogramDx> &Dx) {
  m_histogram.setDx(Dx);
}

/// Returns the x data
MantidVec &Histogram1D::dataX() { return m_histogram.dataX(); }

/// Returns the dx data
MantidVec &Histogram1D::dataDx() { return m_histogram.dataDx(); }

/// Returns the x data const
const MantidVec &Histogram1D::dataX() const { return m_histogram.dataX(); }

/// Returns the dx data const
const MantidVec &Histogram1D::dataDx() const { return m_histogram.dataDx(); }

/// Returns the x data const
const MantidVec &Histogram1D::readX() const { return m_histogram.readX(); }

/// Returns the dx data const
const MantidVec &Histogram1D::readDx() const { return m_histogram.readDx(); }

/// Returns a pointer to the x data
Kernel::cow_ptr<HistogramData::HistogramX> Histogram1D::ptrX() const {
  return m_histogram.ptrX();
}

/// Returns a pointer to the dx data
Kernel::cow_ptr<HistogramData::HistogramDx> Histogram1D::ptrDx() const {
  return m_histogram.ptrDx();
}

} // namespace DataObjects
} // namespace Mantid
