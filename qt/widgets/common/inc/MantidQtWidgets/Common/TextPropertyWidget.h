// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/PropertyWidget.h"
#include <QLabel>
#include <QLineEdit>

namespace MantidQt {
namespace API {

/** The most generic widgets for Property's that are only
 * a simple string.

  @date 2012-02-16
*/
class DLLExport TextPropertyWidget : public PropertyWidget {
  Q_OBJECT

public:
  TextPropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent = nullptr, QGridLayout *layout = nullptr,
                     int row = -1);
  ~TextPropertyWidget() override;
  QString getValue() const override;
  void setValueImpl(const QString &value) override;

  ///@return the main widget of this combo of widgets
  QWidget *getMainWidget() override { return m_textbox; }

protected:
  /// Label (name of the property)
  QLabel *m_label;

  /// The text box to edit
  QLineEdit *m_textbox;

private:
};

} // namespace API
} // namespace MantidQt
