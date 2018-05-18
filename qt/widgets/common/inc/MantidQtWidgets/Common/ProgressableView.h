#ifndef MANTIDQTMANTIDWIDGETS_PROGRESSABLEVIEW_H_
#define MANTIDQTMANTIDWIDGETS_PROGRESSABLEVIEW_H_

#include "MantidQtWidgets/Common/DllOption.h"

namespace MantidQt {
namespace MantidWidgets {

/** ProgressableView : Abstract view useful for indicating progress

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTIDQT_COMMON ProgressableView {
public:
  enum class Style { PERCENTAGE, ENDLESS };

  ProgressableView() : m_style{Style::PERCENTAGE}, m_min(0), m_max(100) {}
  virtual ~ProgressableView() {}

  virtual void setProgress(int progress) = 0;
  virtual void clearProgress() = 0;

  virtual void setProgressRange(int min, int max) {
    // Cache values for a percentage-style progress bar i.e. where both are not
    // zero
    if (min != 0 || max != 0) {
      m_min = min;
      m_max = max;
    }
  }

  bool isPercentageIndicator() { return m_style == Style::PERCENTAGE; }
  void setStyle(Style style) {
    m_style = style;
    // To get QProgressBar to display as an endless progress indicator, we need
    // to set start=end=0
    if (m_style == Style::ENDLESS)
      setProgressRange(0, 0);
    else
      setProgressRange(m_min, m_max);
  };

private:
  Style m_style;
  int m_min;
  int m_max;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQTMANTIDWIDGETS_PROGRESSABLEVIEW_H_ */
