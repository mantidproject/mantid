// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidHistogramData/HistogramBuilder.h"

namespace Mantid {
namespace HistogramData {

/// Set the `distribution` property. true = Frequencies, false = Counts. If not
/// set, the default is Counts.
void HistogramBuilder::setDistribution(bool isDistribution) {
  m_isDistribution = isDistribution;
}

/// Return a Histogram based on previously set information. Throws if
/// information is incomplete are inconsisten.
Histogram HistogramBuilder::build() const {
  if (!m_x)
    throw std::runtime_error("HistogramBuilder: No X data has been set");
  if (!m_y)
    throw std::runtime_error("HistogramBuilder: No Y data has been set");

  std::unique_ptr<Histogram> histogram;
  if (getHistogramXMode(m_x->size(), m_y->size()) ==
      Histogram::XMode::BinEdges) {
    if (m_isDistribution)
      histogram = std::make_unique<Histogram>(BinEdges(m_x), Frequencies(m_y));
    else
      histogram = std::make_unique<Histogram>(BinEdges(m_x), Counts(m_y));
  } else {
    if (m_isDistribution)
      histogram = std::make_unique<Histogram>(Points(m_x), Frequencies(m_y));
    else
      histogram = std::make_unique<Histogram>(Points(m_x), Counts(m_y));
  }
  if (m_e)
    histogram->setSharedE(m_e);
  if (d_x)
    histogram->setSharedDx(d_x);
  return *histogram;
}

} // namespace HistogramData
} // namespace Mantid
