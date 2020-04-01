// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QDialog>
#include <QPointF>

namespace Ui {
class UCorrectionDialog;
}

namespace MantidQt {
namespace MantidWidgets {

class UCorrectionDialog : public QDialog {
  Q_OBJECT
public:
  UCorrectionDialog(QWidget *parent, QPointF oldValue, bool isManual);
  ~UCorrectionDialog() override;

  bool applyCorrection() const;
  QPointF getValue() const;

private:
  Ui::UCorrectionDialog *ui;
};

} // namespace MantidWidgets
} // namespace MantidQt
