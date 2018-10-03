#include "MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h"

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
#pragma warning disable 1170
#endif

#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqPVApplicationCore.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#if defined(__INTEL_COMPILER)
#pragma warning enable 1170
#endif

namespace Mantid {
namespace Vates {
namespace SimpleGui {

/**
 * @param parent the parent widget for the time control widget
 */
TimeControlWidget::TimeControlWidget(QWidget *parent) : QWidget(parent) {
  this->ui.setupUi(this);
}

TimeControlWidget::~TimeControlWidget() {}

/**
 * Function to update the animation scene with "time" information. This
 * updates the animation controls automatically. The "time" information
 * can be any fourth dimension to the dataset, i.e. energy transfer.
 * @param timeStart the start "time" for the data
 * @param timeEnd the end "time" for the data
 * @param numTimesteps the number of "time" steps for the data
 */
void TimeControlWidget::updateAnimationControls(double timeStart,
                                                double timeEnd,
                                                int numTimesteps) {
  pqAnimationScene *scene =
      pqPVApplicationCore::instance()->animationManager()->getActiveScene();
  vtkSMProxy *proxy = scene->getProxy();
  vtkSMPropertyHelper(proxy, "StartTime").Set(timeStart);
  vtkSMPropertyHelper(proxy, "EndTime").Set(timeEnd);
  vtkSMPropertyHelper(proxy, "NumberOfFrames").Set(numTimesteps);
  proxy->InvokeCommand("GoToFirst");
}

/**
 * Function to enable or disable the entire animation controls widget.
 * @param state how to set the animation controls
 */
void TimeControlWidget::enableAnimationControls(bool state) {
  this->setEnabled(state);
}
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
