#include "MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h"
#include "MantidKernel/Logger.h"

#include <map>
#include <algorithm>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
namespace
{
    /// Static logger
    Kernel::Logger g_log("MdControlWidget");
}

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

  // Add the mapping from string to the view enum
  mapFromStringToView.insert(std::pair<std::string ,ModeControlWidget::Views>("STANDARD", ModeControlWidget::STANDARD));
  mapFromStringToView.insert(std::pair<std::string ,ModeControlWidget::Views>("THREESLICE", ModeControlWidget::THREESLICE));
  mapFromStringToView.insert(std::pair<std::string ,ModeControlWidget::Views>("MULTISLICE", ModeControlWidget::MULTISLICE));
  mapFromStringToView.insert(std::pair<std::string ,ModeControlWidget::Views>("SPLATTERPLOT", ModeControlWidget::SPLATTERPLOT));
}

ModeControlWidget::~ModeControlWidget()
{
  
}

void ModeControlWidget::enableViewButtons(ModeControlWidget::Views initialView, bool state)
{
  // Set all buttons to the specified state
  this->ui.standardButton->setEnabled(state);
  this->ui.multiSliceButton->setEnabled(state);
  this->ui.splatterPlotButton->setEnabled(state);
  this->ui.threeSliceButton->setEnabled(state);

  // Disable the defaultView (this is already disabled in the case of state == false)
  switch(initialView)
  {
    case ModeControlWidget::STANDARD:
    {
      this->ui.standardButton->setEnabled(false);
    }
    break;

    case ModeControlWidget::THREESLICE:
    {
      this->ui.threeSliceButton->setEnabled(false);
    }
    break;

    case ModeControlWidget::MULTISLICE:
    {
      this->ui.multiSliceButton->setEnabled(false);
    }
    break;

    case ModeControlWidget::SPLATTERPLOT:
    {
      this->ui.splatterPlotButton->setEnabled(false);
    }
    break;

    default:
      g_log.warning("Attempted to disable an unknown default view. \n");
      break;
  }
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
 * Set the current view to a new, selected view
 *
 */
void ModeControlWidget::setToSelectedView(ModeControlWidget::Views view)
{
  switch(view)
  {
    case ModeControlWidget::STANDARD:
    {
      this->onStandardViewButtonClicked();
    }
    break;

    case ModeControlWidget::MULTISLICE:
    {
      this->onMultiSliceViewButtonClicked();
    }
    break;

    case ModeControlWidget::THREESLICE:
    {
      this->onThreeSliceViewButtonClicked();
    }
    break;

    case ModeControlWidget::SPLATTERPLOT:
    {
      this->onSplatterPlotViewButtonClicked();
    }
    break;

    default:
      break;
  }
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

/**
 * A string is checked against the enum for the views. 
 * @param view A selected view.
 * @returns The selected view as enum or the standard view.
 */
ModeControlWidget::Views ModeControlWidget::getViewFromString(std::string view)
{
  std::transform(view.begin(), view.end(), view.begin(), toupper);

  if (!view.empty() && mapFromStringToView.count(view) == 1)
  {
    return mapFromStringToView[view];
  }
  else 
  {
    // The view was not found, hence return the standard view
    g_log.warning("The specified default view could not be found! \n");

    return ModeControlWidget::STANDARD;
  }
}

} // SimpleGui
} // Vates
} // Mantid
