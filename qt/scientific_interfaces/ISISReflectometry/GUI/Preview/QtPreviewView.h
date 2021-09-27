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
  void plotInstView(std::shared_ptr<MantidWidgets::RotationSurface> &surface) override;

  void setInstViewSelectRectState(bool on) override;
  void setInstViewPanState(bool on) override;
  void setInstViewZoomState(bool on) override;

private:
  Ui::PreviewWidget m_ui;
  PreviewViewSubscriber *m_notifyee{nullptr};
  std::unique_ptr<MantidQt::MantidWidgets::InstrumentDisplay> m_instDisplay{nullptr};

  void connectSignals() const;

private slots:
  void onLoadWorkspaceRequested() const;
  void onInstViewSelectRectClicked() const;
  void onInstViewPanClicked() const;
  void onInstViewZoomClicked() const;
  void loadToolbarIcons();
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
