#include "MantidQtWidgets/InstrumentView/UCorrectionDialog.h"
#include "ui_UCorrectionDialog.h"

namespace MantidQt {
namespace MantidWidgets {

UCorrectionDialog::UCorrectionDialog(QWidget *parent, QPointF oldValue,
                                     bool isManual)
    : QDialog(parent), ui(new Ui::UCorrectionDialog) {
  ui->setupUi(this);
  connect(ui->cbApply, SIGNAL(toggled(bool)), ui->dsbUMin,
          SLOT(setEnabled(bool)));
  connect(ui->cbApply, SIGNAL(toggled(bool)), ui->dsbUMax,
          SLOT(setEnabled(bool)));
  ui->cbApply->setChecked(isManual);
  ui->dsbUMin->setValue(oldValue.x());
  ui->dsbUMax->setValue(oldValue.y());
  ui->dsbUMin->setEnabled(isManual);
  ui->dsbUMax->setEnabled(isManual);
}

UCorrectionDialog::~UCorrectionDialog() { delete ui; }

/**
 * If true the manual correction returned by getValue() method should
 * be applied to the surface or use automatic correction if false.
 */
bool UCorrectionDialog::applyCorrection() const {
  return ui->cbApply->isChecked();
}

/**
 * Get the value of the manual u-correction.
 */
QPointF UCorrectionDialog::getValue() const {
  return QPointF(ui->dsbUMin->value(), ui->dsbUMax->value());
}

} // namespace MantidWidgets
} // namespace MantidQt
