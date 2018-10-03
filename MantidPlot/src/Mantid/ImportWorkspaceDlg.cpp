// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include <QtGui>
#include <qfiledialog.h>

#include "ImportWorkspaceDlg.h"

ImportWorkspaceDlg::ImportWorkspaceDlg(QWidget *parent, size_t num)
    : QDialog(parent), numHists(num), lowerLimit(0), upperLimit(0),
      filtered(false), minValue(0), maxValue(100.) {
  label = new QLabel(
      QString("Set Histogram Range to Load (Max Number = %1):").arg(numHists));

  labelLow = new QLabel(tr("From:"));
  lineLow = new QLineEdit;
  lineLow->setText("0");
  labelLow->setBuddy(lineLow);

  labelHigh = new QLabel(tr("To:"));
  lineHigh = new QLineEdit;
  lineHigh->setText(QString::number(numHists));
  labelHigh->setBuddy(lineHigh);

  checkFilter = new QCheckBox(tr("Set range for 2D plots"));

  QLabel *labelMinimum = new QLabel(tr("Minimum value"));
  lineMinimum = new QLineEdit;

  QLabel *labelMaximum = new QLabel(tr("Maximum value"));
  lineMaximum = new QLineEdit;

  QVBoxLayout *filterLayout = new QVBoxLayout;
  filterLayout->addWidget(checkFilter);

  QGridLayout *grid1 = new QGridLayout;
  grid1->addWidget(labelMinimum, 0, 0);
  grid1->addWidget(lineMinimum, 0, 1);
  grid1->addWidget(labelMaximum, 1, 0);
  grid1->addWidget(lineMaximum, 1, 1);
  filterLayout->addLayout(grid1);

  enableFilter(Qt::Unchecked);

  okButton = new QPushButton(tr("OK"));
  okButton->setDefault(true);
  cancelButton = new QPushButton(tr("Cancel"));

  connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
  connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
  connect(checkFilter, SIGNAL(stateChanged(int)), this,
          SLOT(enableFilter(int)));

  // Set the appearance
  QHBoxLayout *topRowLayout = new QHBoxLayout;
  topRowLayout->addWidget(label);

  QHBoxLayout *middleRowLayout = new QHBoxLayout;
  middleRowLayout->addWidget(labelLow);
  middleRowLayout->addWidget(lineLow);
  middleRowLayout->addWidget(labelHigh);
  middleRowLayout->addWidget(lineHigh);

  QHBoxLayout *bottomRowLayout = new QHBoxLayout;
  bottomRowLayout->addStretch();
  bottomRowLayout->addWidget(cancelButton);
  bottomRowLayout->addWidget(okButton);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(topRowLayout);
  mainLayout->addLayout(middleRowLayout);
  mainLayout->addLayout(filterLayout);
  mainLayout->addLayout(bottomRowLayout);

  setLayout(mainLayout);
  setWindowTitle(tr("Set Histogram Range"));
  setFixedHeight(sizeHint().height());
}

ImportWorkspaceDlg::~ImportWorkspaceDlg() {}

void ImportWorkspaceDlg::okClicked() {
  if (!lineLow->text().isNull() && !lineLow->text().isEmpty() &&
      !lineHigh->text().isNull() && !lineHigh->text().isEmpty()) {
    // Check range is valid
    bool ok;
    int low = lineLow->text().toInt(&ok, 10);

    if (!ok || low < 0 || low > static_cast<int>(numHists)) {
      QMessageBox::warning(this, tr("Mantid"),
                           tr("Lower limit is not valid - please change it.\n"),
                           QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    int high = lineHigh->text().toInt(&ok, 10);

    if (!ok || high < 0 || high > static_cast<int>(numHists)) {
      QMessageBox::warning(this, tr("Mantid"),
                           tr("Upper limit is not valid - please change it.\n"),
                           QMessageBox::Ok, QMessageBox::Ok);
      return;
    }

    if (high < low) {
      lowerLimit = high;
      upperLimit = low;
    } else {
      lowerLimit = low;
      upperLimit = high;
    }

    if (checkFilter->checkState() == Qt::Checked) {
      filtered = true;
      minValue = lineMinimum->text().toDouble(&ok);
      maxValue = lineMaximum->text().toDouble(&ok);
      if (!ok || maxValue < 0) {
        QMessageBox::warning(
            this, tr("Mantid"),
            tr("Maximum value is not valid - please change it.\n"),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
      }
    } else {
      filtered = false;
      maxValue = 0.;
    }

    accept();
  }
}

void ImportWorkspaceDlg::enableFilter(int state) {
  if (state == Qt::Unchecked) {
    lineMaximum->setReadOnly(true);
    lineMinimum->setReadOnly(true);
    lineMinimum->setText("");
    lineMaximum->setText("");
  }
  if (state == Qt::Checked) {
    lineMinimum->setReadOnly(false);
    lineMaximum->setReadOnly(false);
    lineMinimum->setText(QString::number(minValue));
    lineMaximum->setText(QString::number(maxValue));
  }
}
