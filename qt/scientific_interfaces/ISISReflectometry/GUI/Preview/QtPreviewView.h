// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IPreviewView.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/ImageInfoWidgetMini.h"
#include "MantidQtWidgets/InstrumentView/InstrumentDisplay.h"
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"
#include "MantidQtWidgets/RegionSelector/RegionSelector.h"
#include "ui_PreviewWidget.h"
#include <QObject>
#include <QWidget>

#include <memory>
#include <string>

namespace MantidQt::MantidWidgets {
class IPlotView;
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** QtInstrumentView : Provides an interface for the "Preview" tab in the
ISIS Reflectometry interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL QtPreviewView final : public QWidget, public IPreviewView {
  Q_OBJECT
public:
  QtPreviewView(QWidget *parent = nullptr);

  void subscribe(PreviewViewSubscriber *notifyee) noexcept override;

  QLayout *getDockedWidgetsLayout() noexcept override;
  MantidWidgets::IImageInfoWidget *getImageInfo() noexcept override;

  void enableMainWidget() override;
  void disableMainWidget() override;

  std::string getWorkspaceName() const override;
  double getAngle() const override;
  void setAngle(double angle) override;
  void setUpdateAngleButtonEnabled(bool enable) override;
  void setTitle(const std::string &title) override;

private:
  Ui::PreviewWidget m_ui;
  PreviewViewSubscriber *m_notifyee{nullptr};
  std::unique_ptr<MantidQt::MantidWidgets::InstrumentDisplay> m_instDisplay{nullptr};
  MantidQt::MantidWidgets::ImageInfoWidgetMini *m_imageInfo{nullptr};

  void connectSignals() const;

private slots:
  void onLoadWorkspaceRequested() const;
  void onUpdateClicked() const;
  void onAngleEdited();
  void onApplyClicked() const;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
