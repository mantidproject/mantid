#include "ColorSelectionWidget.h"

#include <QDoubleValidator>

ColorSelectionWidget::ColorSelectionWidget(QWidget *parent) : QWidget(parent)
{
  this->ui.setupUi(this);
  this->ui.maxValLineEdit->setValidator(new QDoubleValidator(this));
  this->ui.minValLineEdit->setValidator(new QDoubleValidator(this));

  QObject::connect(this->ui.autoColorScaleCheckBox, SIGNAL(stateChanged(int)),
      this, SLOT(autoOrManualScaling(int)));
}

void ColorSelectionWidget::autoOrManualScaling(int state)
{
  switch (state)
  {
  case Qt::Unchecked:
    this->ui.maxValLabel->setEnabled(true);
    this->ui.maxValLineEdit->setEnabled(true);
    this->ui.minValLabel->setEnabled(true);
    this->ui.minValLineEdit->setEnabled(true);
    break;
  case Qt::Checked:
    this->ui.maxValLabel->setEnabled(false);
    this->ui.maxValLineEdit->setEnabled(false);
    this->ui.minValLabel->setEnabled(false);
    this->ui.minValLineEdit->setEnabled(false);
    break;
  }
}
