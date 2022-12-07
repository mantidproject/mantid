// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IPreviewDockedWidgets.h"
#include "MantidQtWidgets/InstrumentView/InstrumentDisplay.h"
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"
#include "MantidQtWidgets/RegionSelector/RegionSelector.h"
#include "ui_PreviewDockedWidgets.h"
#include <QMainWindow>
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
class MANTIDQT_ISISREFLECTOMETRY_DLL QtPreviewDockedWidgets final : public QMainWindow, public IPreviewDockedWidgets {
  Q_OBJECT
public:
  QtPreviewDockedWidgets(QWidget *parent = nullptr, QLayout *layout = nullptr);

  void subscribe(PreviewDockedWidgetsSubscriber *notifyee) noexcept override;

  // Plotting
  void resetInstView() override;
  void plotInstView(MantidWidgets::InstrumentActor *instActor, Mantid::Kernel::V3D const &samplePos,
                    Mantid::Kernel::V3D const &axis) override;
  // Instrument viewer toolbar
  void setInstViewZoomState(bool isChecked) override;
  void setInstViewEditState(bool isChecked) override;
  void setInstViewSelectRectState(bool isChecked) override;
  void setInstViewZoomMode() override;
  void setInstViewEditMode() override;
  void setInstViewSelectRectMode() override;
  void setInstViewToolbarEnabled(bool enable) override;
  void setRegionSelectorEnabled(bool enable) override;
  // Region selector toolbar
  void setEditROIState(bool state) override;
  void setRectangularROIState(bool state) override;

  std::vector<size_t> getSelectedDetectors() const override;
  std::string getRegionType() const override;

  QLayout *getRegionSelectorLayout() const override;
  MantidQt::MantidWidgets::IPlotView *getLinePlotView() const override;

private:
  Ui::PreviewDockedWidgets m_ui;
  QLayout *m_layout;
  PreviewDockedWidgetsSubscriber *m_notifyee{nullptr};
  std::unique_ptr<MantidQt::MantidWidgets::InstrumentDisplay> m_instDisplay{nullptr};

  void connectSignals() const;
  void loadToolbarIcons();
  void setupSelectRegionTypes();

private slots:
  void onInstViewSelectRectClicked() const;
  void onInstViewZoomClicked() const;
  void onInstViewEditClicked() const;
  void onInstViewShapeChanged() const;
  void onRegionSelectorExportToAdsClicked() const;
  void onLinePlotExportToAdsClicked() const;
  void onEditROIClicked() const;
  void onAddRectangularROIClicked(QAction *regionType) const;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
