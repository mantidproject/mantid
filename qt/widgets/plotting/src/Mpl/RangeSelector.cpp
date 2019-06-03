// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/RangeSelector.h"
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlot.h"

namespace MantidQt {
namespace MantidWidgets {

RangeSelector::RangeSelector(PreviewPlot *plot, SelectType type, bool visible,
                             bool infoOnly)
    : QObject(), m_type(type) {
  Q_UNUSED(plot);
  Q_UNUSED(visible);
  Q_UNUSED(infoOnly);
}

void RangeSelector::setRange(const std::pair<double, double> &range) {
  setRange(range.first, range.second);
}

void RangeSelector::setRange(const double min, const double max) {
  Q_UNUSED(min);
  Q_UNUSED(max);
  throw std::runtime_error("RangeSelector::setRange not implemented");
}

void RangeSelector::setMinimum(double value) {
  if (value != m_min)
    m_min = (value > m_lower) ? value : m_lower;
}

void RangeSelector::setMaximum(double value) {
  if (value != m_max)
    m_max = (value < m_higher) ? value : m_higher;
}

double RangeSelector::getMinimum() { return m_min; }

double RangeSelector::getMaximum() { return m_max; }

void RangeSelector::detach() {}

void RangeSelector::setColour(QColor colour) {
  // m_pen->setColor(colour);
  // switch (m_type) {
  // case XMINMAX:
  // case YMINMAX:
  //  m_mrkMax->setLinePen(*m_pen);
  // case XSINGLE:
  // case YSINGLE:
  //  m_mrkMin->setLinePen(*m_pen);
  //  break;
  //}
  Q_UNUSED(colour);
}

} // namespace MantidWidgets
} // namespace MantidQt
