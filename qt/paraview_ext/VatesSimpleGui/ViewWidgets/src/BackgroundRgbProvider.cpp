// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesSimpleGuiViewWidgets/BackgroundRgbProvider.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/MdSettings.h"

#include <array>
#include <cmath>
#include <vector>

#include <pqRenderView.h>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMViewProxy.h>
#include <vtkSmartPointer.h>

namespace Mantid {
namespace Vates {
namespace SimpleGui {

QColor BackgroundRgbProvider::currentBackgroundColor = QColor(84, 89, 109);

BackgroundRgbProvider::BackgroundRgbProvider() {}

BackgroundRgbProvider::~BackgroundRgbProvider() {
  // Need to record the background color
  update();
}

std::vector<double>
BackgroundRgbProvider::getRgb(bool useCurrentBackgroundColor) {
  // Get the rgb setting from the config file
  std::vector<double> userSettingRgb =
      getRgbFromSetting(useCurrentBackgroundColor);

  // Normalize the entries to 256
  userSettingRgb[0] = userSettingRgb[0] / 255.0;
  userSettingRgb[1] = userSettingRgb[1] / 255.0;
  userSettingRgb[2] = userSettingRgb[2] / 255.0;

  return userSettingRgb;
}

std::vector<double>
BackgroundRgbProvider::getRgbFromSetting(bool useCurrentBackgroundColor) {
  // Set the mantid default here
  QColor userBackground;

  if (useCurrentBackgroundColor) {
    // Update the settings
    update();

    userBackground = m_mdSettings.getLastSessionBackgroundColor();
  } else {
    if (m_mdSettings.getUsageLastSession()) {
      userBackground = m_mdSettings.getLastSessionBackgroundColor();
    } else {
      // Select the user setting as the background color and make the user
      // setting the last session color
      userBackground = m_mdSettings.getUserSettingBackgroundColor();

      m_mdSettings.setLastSessionBackgroundColor(userBackground);
    }

    // Need to make sure that the static variable is initialized correctly, else
    // it will show a black background
    currentBackgroundColor = userBackground;
  }

  // Get the background
  int rVal;
  int gVal;
  int bVal;

  if (userBackground.isValid()) {
    rVal = userBackground.red();
    gVal = userBackground.green();
    bVal = userBackground.blue();
  } else {
    // Set the default
    QColor defaultBackgroundColor = m_mdSettings.getDefaultBackgroundColor();
    rVal = defaultBackgroundColor.red();
    gVal = defaultBackgroundColor.green();
    bVal = defaultBackgroundColor.blue();
  }

  return {static_cast<double>(rVal), static_cast<double>(gVal),
          static_cast<double>(bVal)};
}

void BackgroundRgbProvider::update() {
  m_mdSettings.setLastSessionBackgroundColor(currentBackgroundColor);
}

void BackgroundRgbProvider::setBackgroundColor(pqRenderView *view,
                                               bool useCurrentBackgroundColor) {
  std::vector<double> backgroundRgb = getRgb(useCurrentBackgroundColor);

  vtkSMDoubleVectorProperty *background =
      vtkSMDoubleVectorProperty::SafeDownCast(
          view->getViewProxy()->GetProperty("Background"));

  background->SetElements3(backgroundRgb[0], backgroundRgb[1],
                           backgroundRgb[2]);

  view->resetCamera();
}

void BackgroundRgbProvider::observe(pqRenderView *view) {
  // For more information http://www.vtk.org/Wiki/VTK/Tutorials/Callbacks
  vtkSmartPointer<vtkCallbackCommand> backgroundColorChangeCallback =
      vtkSmartPointer<vtkCallbackCommand>::New();
  backgroundColorChangeCallback->SetCallback(
      backgroundColorChangeCallbackFunction);

  view->getViewProxy()
      ->GetProperty("Background")
      ->AddObserver(vtkCommand::ModifiedEvent, backgroundColorChangeCallback);
}

void BackgroundRgbProvider::backgroundColorChangeCallbackFunction(
    vtkObject *caller, long unsigned int, void *, void *) {
  // Extract the background color and persist it
  vtkSMDoubleVectorProperty *background =
      vtkSMDoubleVectorProperty::SafeDownCast(caller);

  int numberOfElements = background->GetNumberOfElements();
  double *elements = background->GetElements();

  if (numberOfElements >= 3) {
    double r = elements[0] * 255.0;
    double g = elements[1] * 255.0;
    double b = elements[2] * 255.0;

    int red = static_cast<int>(r);
    int green = static_cast<int>(g);
    int blue = static_cast<int>(b);

    currentBackgroundColor = QColor(red, green, blue);
  }
}
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
