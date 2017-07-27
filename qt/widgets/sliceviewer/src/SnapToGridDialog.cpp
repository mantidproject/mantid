#include "MantidQtSliceViewer/SnapToGridDialog.h"
#include <QIntValidator>

/// Constructor
SnapToGridDialog::SnapToGridDialog(QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);
  ui.txtSnapX->setValidator(new QDoubleValidator(ui.txtSnapX));
  ui.txtSnapY->setValidator(new QDoubleValidator(ui.txtSnapY));

  QObject::connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(accept()));
  QObject::connect(ui.btnCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

/// Set the initial snap-to values.
void SnapToGridDialog::setSnap(double x, double y) {
  ui.txtSnapX->setText(QString::number(x));
  ui.txtSnapY->setText(QString::number(y));
}

///@return the snap-to-X value
double SnapToGridDialog::getSnapX() {
  bool ok;
  double ret = ui.txtSnapX->text().toDouble(&ok);
  if (!ok)
    ret = 0.0;
  return ret;
}

///@return the snap-to-Y value
double SnapToGridDialog::getSnapY() {
  bool ok;
  double ret = ui.txtSnapY->text().toDouble(&ok);
  if (!ok)
    ret = 0.0;
  return ret;
}

SnapToGridDialog::~SnapToGridDialog() {}
