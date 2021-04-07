// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Property.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/PropertyWidget.h"
#include <QComboBox>
#include <QLabel>

namespace MantidQt {
namespace API {

/** Widget for displaying a Property that has a set of allowed values.
 * The display is then a drop-down box instead of a Text box.

  @date 2012-02-17
*/

class DLLExport OptionsPropertyWidget : public PropertyWidget {
  Q_OBJECT

public:
  OptionsPropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent = nullptr, QGridLayout *layout = nullptr,
                        int row = -1);
  ~OptionsPropertyWidget() override;
  QString getValue() const override;
  void setValueImpl(const QString &value) override;

  ///@return the main widget of this combo of widgets
  QWidget *getMainWidget() override { return m_combo; }

protected:
  /// Label (name of the property)
  QLabel *m_label;

  /// Combo box with the allowed options
  QComboBox *m_combo;

public slots:
  void editingFinished();
};

} // namespace API
} // namespace MantidQt
