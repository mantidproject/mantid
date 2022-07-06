// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"

#include <QLayout>
#include <memory>
#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class PreviewViewSubscriber {
public:
  virtual ~PreviewViewSubscriber() = default;
  virtual void notifyLoadWorkspaceRequested() = 0;

  virtual void notifyInstViewZoomRequested() = 0;
  virtual void notifyInstViewEditRequested() = 0;
  virtual void notifyInstViewSelectRectRequested() = 0;
  virtual void notifyInstViewShapeChanged() = 0;

  virtual void notifyRegionSelectorExportAdsRequested() = 0;
  virtual void notify1DPlotExportAdsRequested() = 0;

  // TODO implement edit ROI button and ROI-changed callback
  // virtual void notifyEditROIRequested() = 0;
  // virtual void notifyROIChanged() = 0;
  virtual void notifyRectangularROIModeRequested() = 0;
};

class IPreviewView {
public:
  virtual ~IPreviewView() = default;
  virtual void subscribe(PreviewViewSubscriber *notifyee) noexcept = 0;
  virtual std::string getWorkspaceName() const = 0;
  virtual double getAngle() const = 0;
  // Plotting
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
  // Region selector toolbar
  // TODO implement edit ROI button
  // virtual void setEditROIState(bool on) = 0;
  // virtual void activateEditROIMode() = 0;
  virtual void setRectangularROIState(bool on) = 0;

  virtual std::vector<size_t> getSelectedDetectors() const = 0;

  virtual QLayout *getRegionSelectorLayout() const = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
