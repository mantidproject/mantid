// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPreviewInstrumentDisplay.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "ShapeChangedRelay.h"
#include <functional>

using namespace MantidQt::Widgets::Common;

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class MANTIDQT_ISISREFLECTOMETRY_DLL PreviewPythonInstrumentView : public Python::InstanceHolder,
                                                                   public IPreviewInstrumentDisplay {
public:
  PreviewPythonInstrumentView(QLayout *layout = nullptr);
  PreviewPythonInstrumentView(PreviewPythonInstrumentView const &) = delete;
  PreviewPythonInstrumentView(PreviewPythonInstrumentView &&);
  PreviewPythonInstrumentView &operator=(PreviewPythonInstrumentView const &) = delete;
  PreviewPythonInstrumentView &operator=(PreviewPythonInstrumentView &&);

  void setLayout(QLayout *layout);

  void setShapeChangedCallback(std::function<void()> callback);

  void updateWorkspace(Mantid::API::MatrixWorkspace_sptr &workspace) override;
  void resetInstView() override;
  void plotInstView() override;
  void setInstViewZoomMode() override;
  void setInstViewEditMode() override {};
  void setInstViewSelectRectMode() override;
  std::vector<Mantid::detid_t> getSelectedDetectorIDs() const override;

private:
  Python::Object getView() const;

  QLayout *m_layout;
  std::unique_ptr<ShapeChangedRelay> m_relay;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
