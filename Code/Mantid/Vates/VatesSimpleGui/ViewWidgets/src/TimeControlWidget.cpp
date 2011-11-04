#include "MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h"

#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqPVApplicationCore.h>
#include <vtkSMPropertyHelper.h>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

TimeControlWidget::TimeControlWidget(QWidget *parent) : QWidget(parent)
{
    this->ui.setupUi(this);
}

TimeControlWidget::~TimeControlWidget()
{
}

void TimeControlWidget::updateAnimationControls(double timeStart,
                                                double timeEnd,
                                                int numTimesteps)
{
  pqAnimationScene *scene = pqPVApplicationCore::instance()->animationManager()->getActiveScene();
  vtkSMPropertyHelper(scene->getProxy(), "StartTime").Set(timeStart);
  vtkSMPropertyHelper(scene->getProxy(), "EndTime").Set(timeEnd);
  vtkSMPropertyHelper(scene->getProxy(), "NumberOfFrames").Set(numTimesteps);
}

/**
 * Function to enable or disable the entire animation controls widget.
 * @param state how to set the animation controls
 */
void TimeControlWidget::enableAnimationControls(bool state)
{
  this->setEnabled(state);
}

}
}
}
