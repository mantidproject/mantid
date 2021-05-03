// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidKernel/Statistics.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "ui_LogValueSelector.h"

namespace MantidQt {
namespace MantidWidgets {

/** LogValueSelector : Select a log name and mean/min/max/first/last
 */
class EXPORT_OPT_MANTIDQT_COMMON LogValueSelector : public MantidQt::API::MantidWidget {
  Q_OBJECT
public:
  /// Constructor
  LogValueSelector(QWidget *parent);

  /// Get selected log text
  QString getLog() const;

  /// Get selected function text
  QString getFunctionText() const;

  /// Get selected function enum value
  Mantid::Kernel::Math::StatisticType getFunction() const;

  /// Whether checkbox is shown or not
  bool isCheckboxShown() const;

  /// Control whether checkbox is shown
  void setCheckboxShown(bool visible);

  /// Get a pointer to log combo box
  QComboBox *getLogComboBox() const;

  /// Set enabled/disabled
  void setEnabled(bool enabled) { this->setEnabled(enabled ? Qt::Checked : Qt::Unchecked); }

  /// Whether checkbox is ticked or not
  bool isCheckboxTicked() const;

signals:
  /// Checkbox state has changed
  void logOptionsEnabled(bool enabled);

private slots:
  /// Set enabled/disabled
  void setEnabled(int checkstate);

private:
  /// Set up connections
  void doConnect();

  /// User interface
  Ui::LogValueSelector m_ui;

  /// Converts strings like "Mean" or "Max" to enum values
  static const std::map<std::string, Mantid::Kernel::Math::StatisticType> STRING_TO_FUNC;
};

} // namespace MantidWidgets
} // namespace MantidQt
