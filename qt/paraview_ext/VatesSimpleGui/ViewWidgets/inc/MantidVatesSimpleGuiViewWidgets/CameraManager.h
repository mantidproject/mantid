// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CAMERAMANAGER_H_
#define CAMERAMANAGER_H_

#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

namespace Mantid {
namespace Vates {
namespace SimpleGui {

/**
 *
  This class handles the camera of the view.

  @date 14/1/2015
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS CameraManager {
public:
  CameraManager();

  ~CameraManager();

  Mantid::VATES::ViewFrustum_const_sptr getCurrentViewFrustum();

  void setCameraToPeak(double xpos, double ypos, double zpos,
                       double peakRadius);
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif
