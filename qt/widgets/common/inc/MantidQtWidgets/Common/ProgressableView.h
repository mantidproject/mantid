// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_PROGRESSABLEVIEW_H_
#define MANTIDQTMANTIDWIDGETS_PROGRESSABLEVIEW_H_

#include "MantidQtWidgets/Common/DllOption.h"

namespace MantidQt {
namespace MantidWidgets {

/** ProgressableView : Abstract view useful for indicating progress
*/
class EXPORT_OPT_MANTIDQT_COMMON ProgressableView {
public:
  /// The style of the progress bar: either a standard percentage progress bar
  /// or an endless busy indicator
  enum class Style { PERCENTAGE, ENDLESS };

  ProgressableView() : m_style{Style::PERCENTAGE}, m_min(0), m_max(100) {}
  virtual ~ProgressableView() {}

  virtual void setProgress(int progress) = 0;
  virtual void clearProgress() = 0;
  virtual void setProgressRange(int min, int max);

  bool isPercentageIndicator() const;
  void setAsPercentageIndicator();
  void setAsEndlessIndicator();

protected:
  Style m_style;
  int m_min;
  int m_max;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQTMANTIDWIDGETS_PROGRESSABLEVIEW_H_ */
