// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>

namespace MantidQt {
namespace MantidWidgets {
/**
  This class subclasses and overwrites QHeaderView methods to enable checkboxes
  to exist in the table header.

  @author Jay Rainey
  @date 15/10/2013
*/

class EXPORT_OPT_MANTIDQT_COMMON CheckboxHeader : public QHeaderView {
  Q_OBJECT

public:
  /// Override QHeaderView constructor.
  explicit CheckboxHeader(Qt::Orientation orientation, QWidget *parent = nullptr);
  void setChecked(bool checked);

signals:
  void toggled(bool checked);

protected:
  /// Overrides QHeaderView allowing checkbox functionality in the first column
  /// of the table.
  void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
  /// Set the checkbox to checked when it is clicked.
  void mousePressEvent(QMouseEvent *event) override;

private:
  /// The area around the checkbox.
  QRect checkBoxRect(const QRect &sourceRect) const;
  /// The state of the checkbox in the column header.
  bool m_checked;
};
} // namespace MantidWidgets
} // namespace MantidQt
