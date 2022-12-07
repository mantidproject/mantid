// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/RotationSurface.h"

#include <QLayout>
#include <memory>
#include <string>

namespace MantidQt::MantidWidgets {
class IPlotView;
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IBatchPresenter;

class PreviewDockedWidgetsSubscriber {
public:
  virtual ~PreviewDockedWidgetsSubscriber() = default;

  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;

  virtual void notifyInstViewZoomRequested() = 0;
  virtual void notifyInstViewEditRequested() = 0;
  virtual void notifyInstViewSelectRectRequested() = 0;
  virtual void notifyInstViewShapeChanged() = 0;

  virtual void notifyRegionSelectorExportAdsRequested() = 0;
  virtual void notifyLinePlotExportAdsRequested() = 0;

  virtual void notifyEditROIModeRequested() = 0;
  virtual void notifyRectangularROIModeRequested() = 0;
};

class IPreviewDockedWidgets {
public:
  virtual ~IPreviewDockedWidgets() = default;
  virtual void subscribe(PreviewDockedWidgetsSubscriber *notifyee) noexcept = 0;

  // Plotting
  virtual void resetInstView() = 0;
  virtual void plotInstView(MantidWidgets::InstrumentActor *instActor, Mantid::Kernel::V3D const &samplePos,
                            Mantid::Kernel::V3D const &axis) = 0;
  //  Instrument viewer toolbar
  virtual void setInstViewZoomState(bool on) = 0;
  virtual void setInstViewEditState(bool on) = 0;
  virtual void setInstViewSelectRectState(bool on) = 0;
  virtual void setInstViewZoomMode() = 0;
  virtual void setInstViewEditMode() = 0;
  virtual void setInstViewSelectRectMode() = 0;
  virtual void setInstViewToolbarEnabled(bool enable) = 0;
  virtual void setRegionSelectorEnabled(bool enable) = 0;
  // Region selector toolbar
  virtual void setEditROIState(bool state) = 0;
  virtual void setRectangularROIState(bool state) = 0;

  virtual std::vector<size_t> getSelectedDetectors() const = 0;
  virtual std::string getRegionType() const = 0;

  virtual QLayout *getRegionSelectorLayout() const = 0;
  virtual MantidQt::MantidWidgets::IPlotView *getLinePlotView() const = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
