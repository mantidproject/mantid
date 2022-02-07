// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/PropertyWidget.h"
#include <QCheckBox>

namespace MantidQt {
namespace API {

/** Set of widgets representing a PropertyWithValue<bool>.
 *

  @date 2012-02-16
*/
class DLLExport BoolPropertyWidget : public PropertyWidget {
  Q_OBJECT

public:
  BoolPropertyWidget(Mantid::Kernel::PropertyWithValue<bool> *prop, QWidget *parent = nullptr,
                     QGridLayout *layout = nullptr, int row = -1);
  ~BoolPropertyWidget() override;
  QString getValue() const override;
  void setValueImpl(const QString &value) override;

  ///@return the main widget of this combo of widgets
  QWidget *getMainWidget() override { return m_checkBox; }

protected:
  /// Checkbox widget
  QCheckBox *m_checkBox;
};

} // namespace API
} // namespace MantidQt
