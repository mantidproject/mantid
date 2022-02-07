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
#include <QLabel>
#include <QListWidget>

namespace MantidQt {
namespace API {

/** Widget for displaying a Property that has a set of allowed values.
 * The display is then a multi selection list box instead of a Text box.
 */
class DLLExport ListPropertyWidget : public PropertyWidget {
  Q_OBJECT

public:
  ListPropertyWidget(Mantid::Kernel::Property *prop, QWidget *parent = nullptr, QGridLayout *layout = nullptr,
                     int row = -1);
  ~ListPropertyWidget() override;
  QString getValue() const override;
  void setValueImpl(const QString &value) override;

  ///@return the main widget of this combo of widgets
  QWidget *getMainWidget() override { return m_list; }

protected:
  /// Label (name of the property)
  QLabel *m_label;

  /// List box with the allowed List
  QListWidget *m_list;
};

} // namespace API
} // namespace MantidQt
