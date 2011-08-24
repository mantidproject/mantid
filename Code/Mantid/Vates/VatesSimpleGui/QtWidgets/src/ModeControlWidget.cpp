#include "MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h"

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

ModeControlWidget::ModeControlWidget(QWidget *parent)
  : QWidget(parent)
{
  this->ui.setupUi(this);

  QObject::connect(parent, SIGNAL(enableMultiSliceViewButton()), this,
                   SLOT(enableMultiSliceViewButton()));
  QObject::connect(parent, SIGNAL(enableThreeSliceViewButton()), this,
                   SLOT(enableThreeSliceViewButton()));
  QObject::connect(this->ui.multiSliceButton, SIGNAL(clicked()),
                   this, SLOT(onMultiSliceViewButtonClicked()));
  QObject::connect(this->ui.standardButton, SIGNAL(clicked()),
                   this, SLOT(onStandardViewButtonClicked()));
  QObject::connect(this->ui.threeSliceButton, SIGNAL(clicked()),
                   this, SLOT(onThreeSliceViewButtonClicked()));
}

ModeControlWidget::~ModeControlWidget()
{

}

void ModeControlWidget::enableThreeSliceViewButton()
{
  this->ui.threeSliceButton->setEnabled(true);
}

void ModeControlWidget::enableMultiSliceViewButton()
{
  this->ui.multiSliceButton->setEnabled(true);
}

void ModeControlWidget::onMultiSliceViewButtonClicked()
{
  emit executeSwitchViews(ModeControlWidget::MULTISLICE);
  this->ui.multiSliceButton->setEnabled(false);
  this->ui.standardButton->setEnabled(true);
  this->ui.threeSliceButton->setEnabled(true);
}

void ModeControlWidget::onStandardViewButtonClicked()
{
  emit executeSwitchViews(ModeControlWidget::STANDARD);
  this->ui.standardButton->setEnabled(false);
  this->ui.multiSliceButton->setEnabled(true);
  this->ui.threeSliceButton->setEnabled(true);
}

void ModeControlWidget::onThreeSliceViewButtonClicked()
{
  emit executeSwitchViews(ModeControlWidget::THREESLICE);
  this->ui.threeSliceButton->setEnabled(false);
  this->ui.multiSliceButton->setEnabled(true);
  this->ui.standardButton->setEnabled(true);
}

}
}
}
