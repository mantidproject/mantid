// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IPreviewView.h"
#include "MantidQtWidgets/InstrumentView/InstrumentDisplay.h"
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"
#include "ui_PreviewWidget.h"

#include <QObject>
#include <QWidget>

#include <memory>
#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** QtInstrumentView : Provides an interface for the "Preview" tab in the
ISIS Reflectometry interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL QtPreviewView final : public QWidget, public IPreviewView {
  Q_OBJECT
public:
  QtPreviewView(QWidget *parent = nullptr);

  void subscribe(PreviewViewSubscriber *notifyee) noexcept override;

  std::string getWorkspaceName() const override;
  void plotInstView(MantidWidgets::InstrumentActor *instActor, Mantid::Kernel::V3D const &samplePos,
                    Mantid::Kernel::V3D const &axis) override;

  void setInstViewZoomState(bool isChecked) override;
  void setInstViewEditState(bool isChecked) override;
  void setInstViewSelectRectState(bool isChecked) override;
  void setInstViewZoomMode() override;
  void setInstViewEditMode() override;
  void setInstViewSelectRectMode() override;
  void setInstViewToolbarEnabled(bool enable) override;

  std::vector<size_t> getSelectedDetectors() const override;

private:
  Ui::PreviewWidget m_ui;
  PreviewViewSubscriber *m_notifyee{nullptr};
  std::unique_ptr<MantidQt::MantidWidgets::InstrumentDisplay> m_instDisplay{nullptr};

  void connectSignals() const;
  void loadToolbarIcons();

private slots:
  void onLoadWorkspaceRequested() const;
  void onInstViewSelectRectClicked() const;
  void onInstViewZoomClicked() const;
  void onInstViewEditClicked() const;
  void onInstViewShapeChanged() const;
  void onContourExportToAdsClicked() const;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
