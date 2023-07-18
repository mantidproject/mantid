// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/IImageInfoWidget.h"
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"

#include <QLayout>
#include <memory>
#include <string>

namespace MantidQt::MantidWidgets {
class IPlotView;
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IBatchPresenter;

class PreviewViewSubscriber {
public:
  virtual ~PreviewViewSubscriber() = default;

  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;

  virtual void notifyLoadWorkspaceRequested() = 0;
  virtual void notifyUpdateAngle() = 0;
  virtual void notifyApplyRequested() = 0;
};

class IPreviewView {
public:
  virtual ~IPreviewView() = default;
  virtual void subscribe(PreviewViewSubscriber *notifyee) noexcept = 0;
  virtual QLayout *getDockedWidgetsLayout() noexcept = 0;
  virtual MantidWidgets::IImageInfoWidget *getImageInfo() noexcept = 0;
  virtual void enableMainWidget() = 0;
  virtual void disableMainWidget() = 0;

  virtual std::string getWorkspaceName() const = 0;
  virtual double getAngle() const = 0;
  virtual void setAngle(double angle) = 0;
  virtual void setUpdateAngleButtonEnabled(bool enabled) = 0;
  virtual void setTitle(const std::string &title) = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
