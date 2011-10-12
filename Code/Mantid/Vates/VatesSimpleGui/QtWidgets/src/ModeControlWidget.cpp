#include "MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h"

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

ModeControlWidget::ModeControlWidget(QWidget *parent) : QWidget(parent)
{
  this->ui.setupUi(this);

  QObject::connect(this->ui.multiSliceButton, SIGNAL(clicked()),
                   this, SLOT(onMultiSliceViewButtonClicked()));
  QObject::connect(this->ui.standardButton, SIGNAL(clicked()),
                   this, SLOT(onStandardViewButtonClicked()));
  QObject::connect(this->ui.threeSliceButton, SIGNAL(clicked()),
                   this, SLOT(onThreeSliceViewButtonClicked()));
  QObject::connect(this->ui.splatterPlotButton, SIGNAL(clicked()),
                   this, SLOT(onSplatterPlotViewButtonClicked()));
}

ModeControlWidget::~ModeControlWidget()
{

}

void ModeControlWidget::enableViewButtons()
{
  this->ui.multiSliceButton->setEnabled(true);
  this->ui.splatterPlotButton->setEnabled(true);
  this->ui.threeSliceButton->setEnabled(true);
}

void ModeControlWidget::onMultiSliceViewButtonClicked()
{
  emit executeSwitchViews(ModeControlWidget::MULTISLICE);
  this->ui.multiSliceButton->setEnabled(false);
  this->ui.standardButton->setEnabled(true);
  this->ui.splatterPlotButton->setEnabled(true);
  this->ui.threeSliceButton->setEnabled(true);
}

void ModeControlWidget::onStandardViewButtonClicked()
{
  emit executeSwitchViews(ModeControlWidget::STANDARD);
  this->ui.standardButton->setEnabled(false);
  this->ui.multiSliceButton->setEnabled(true);
  this->ui.splatterPlotButton->setEnabled(true);
  this->ui.threeSliceButton->setEnabled(true);
}

void ModeControlWidget::onThreeSliceViewButtonClicked()
{
  emit executeSwitchViews(ModeControlWidget::THREESLICE);
  this->ui.threeSliceButton->setEnabled(false);
  this->ui.multiSliceButton->setEnabled(true);
  this->ui.splatterPlotButton->setEnabled(true);
  this->ui.standardButton->setEnabled(true);
}

void ModeControlWidget::setToStandardView()
{
  this->onStandardViewButtonClicked();
}

void ModeControlWidget::onSplatterPlotViewButtonClicked()
{
  this->ui.splatterPlotButton->setEnabled(false);
  this->ui.standardButton->setEnabled(true);
  this->ui.multiSliceButton->setEnabled(true);
  this->ui.threeSliceButton->setEnabled(true);
  emit executeSwitchViews(ModeControlWidget::SPLATTERPLOT);
}

}
}
}
