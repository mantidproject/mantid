#include "MantidQtWidgets/Common/ProgressableView.h"

namespace MantidQt {
namespace MantidWidgets {

bool ProgressableView::isPercentageIndicator() const {
  return m_style == Style::PERCENTAGE;
}

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
  if (m_style == Style::ENDLESS)
    setProgressRange(0, 0);
}
} // namespace MantidWidgets
} // namepsace MantidQt
