// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RemoveErrorsDialog.h"
#include "ui_RemoveErrorsDialog.h"

RemoveErrorsDialog::RemoveErrorsDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::RemoveErrorsDialog) {
  ui->setupUi(this);
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(remove()));
}

RemoveErrorsDialog::~RemoveErrorsDialog() { delete ui; }

void RemoveErrorsDialog::setCurveNames(const QStringList &names) {
  ui->nameLabel->addItems(names);
}

void RemoveErrorsDialog::remove() {
  emit curveName(ui->nameLabel->currentText());
}