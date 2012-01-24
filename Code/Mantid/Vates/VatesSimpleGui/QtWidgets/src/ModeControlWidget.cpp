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

void ModeControlWidget::enableViewButtons(bool state)
{
  this->ui.multiSliceButton->setEnabled(state);
  this->ui.splatterPlotButton->setEnabled(state);
  this->ui.threeSliceButton->setEnabled(state);
}

void ModeControlWidget::onMultiSliceViewButtonClicked()
{
  this->ui.multiSliceButton->setEnabled(false);
  this->ui.standardButton->setEnabled(true);
  this->ui.splatterPlotButton->setEnabled(true);
  this->ui.threeSliceButton->setEnabled(true);
  emit executeSwitchViews(ModeControlWidget::MULTISLICE);
}

void ModeControlWidget::onStandardViewButtonClicked()
{
  this->ui.standardButton->setEnabled(false);
  this->ui.multiSliceButton->setEnabled(true);
  this->ui.splatterPlotButton->setEnabled(true);
  this->ui.threeSliceButton->setEnabled(true);
  emit executeSwitchViews(ModeControlWidget::STANDARD);
}

void ModeControlWidget::onThreeSliceViewButtonClicked()
{
  this->ui.threeSliceButton->setEnabled(false);
  this->ui.multiSliceButton->setEnabled(true);
  this->ui.splatterPlotButton->setEnabled(true);
  this->ui.standardButton->setEnabled(true);
  emit executeSwitchViews(ModeControlWidget::THREESLICE);
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

/**
 * This function allows one to enable/disable a specific view button.
 * @param mode The view mode button to set state for
 * @param state Enable/diable the view mode button
 */
void ModeControlWidget::enableViewButton(ModeControlWidget::Views mode,
                                         bool state)
{
  switch (mode)
  {
  case ModeControlWidget::STANDARD:
    this->ui.standardButton->setEnabled(state);
    break;
  case ModeControlWidget::MULTISLICE:
    this->ui.multiSliceButton->setEnabled(state);
    break;
  case ModeControlWidget::THREESLICE:
    this->ui.threeSliceButton->setEnabled(state);
    break;
  case ModeControlWidget::SPLATTERPLOT:
    this->ui.splatterPlotButton->setEnabled(state);
    break;
  default:
    break;
  }
}

} // SimpleGui
} // Vates
} // Mantid
