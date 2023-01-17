// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include "DetectorTube.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSurface.h"

#include <memory>
#include <vector>

namespace MantidQt::CustomInterfaces {

class MANTIDQT_DIRECT_DLL ALFInstrumentWidget : public MantidWidgets::InstrumentWidget {
  Q_OBJECT

public:
  explicit ALFInstrumentWidget(QString workspaceName);

  void handleActiveWorkspaceDeleted() override;

  std::vector<DetectorTube> findWholeTubeDetectorIndices(std::vector<std::size_t> const &partTubeDetectorIndices);

  void drawRectanglesAbove(std::vector<DetectorTube> const &tubes);

private:
  MantidWidgets::InstrumentWidget::TabCustomizations getTabCustomizations() const;

  void drawRectangleAbove(std::shared_ptr<MantidWidgets::UnwrappedSurface> surface, DetectorTube const &tube);
};

} // namespace MantidQt::CustomInterfaces
