// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ProgressableView.h"

namespace MantidQt::MantidWidgets {

bool ProgressableView::isPercentageIndicator() const { return m_style == Style::PERCENTAGE; }

void ProgressableView::setProgressRange(int min, int max) {
  // Cache values for a percentage-style progress bar i.e. where both are not
  // zero
  if (min != 0 || max != 0) {
    m_min = min;
    m_max = max;
  }
}

void ProgressableView::setAsPercentageIndicator() {
  m_style = Style::PERCENTAGE;
  setProgressRange(m_min, m_max);
}

void ProgressableView::setAsEndlessIndicator() {
  m_style = Style::ENDLESS;
  // To get QProgressBar to display as an endless progress indicator, we need
  // to set start=end=0 in the derived view class
  setProgressRange(0, 0);
}
} // namespace MantidQt::MantidWidgets
