// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IPreviewInstrumentDisplay.h"
#include "InstViewModel.h"
#include "MantidQtWidgets/InstrumentView/InstrumentDisplay.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"

#include <QMetaObject>
#include <functional>
#include <memory>
#include <vector>

class QWidget;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL QtPreviewInstrumentDisplay : public IPreviewInstrumentDisplay {
public:
  QtPreviewInstrumentDisplay(QWidget *placeholder, std::function<void()> onShapeChanged,
                             std::unique_ptr<IInstViewModel> instViewModel);
  ~QtPreviewInstrumentDisplay() override;

  void updateWorkspace(Mantid::API::MatrixWorkspace_sptr &workspace) override;
  void resetInstView() override;
  void plotInstView() override;
  void setInstViewZoomMode() override;
  void setInstViewEditMode() override;
  void setInstViewSelectRectMode() override;
  std::vector<size_t> getSelectedDetectors() const override;
  std::vector<Mantid::detid_t> detIndicesToDetIDs(std::vector<size_t> const &detIndices) const override;

private:
  QWidget *m_placeholder;
  std::unique_ptr<MantidWidgets::InstrumentDisplay> m_instDisplay;
  std::unique_ptr<IInstViewModel> m_instViewModel;
  std::function<void()> m_onShapeChanged;
  std::vector<QMetaObject::Connection> m_surfaceConnections;

  void disconnectSurfaceSignals();
  void connectSurfaceSignals();
  MantidQt::MantidWidgets::ProjectionSurface_sptr getSurface();
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
