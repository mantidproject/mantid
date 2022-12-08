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

#include <vector>

namespace MantidQt::CustomInterfaces {

class MANTIDQT_DIRECT_DLL ALFInstrumentWidget : public MantidWidgets::InstrumentWidget {
  Q_OBJECT

public:
  explicit ALFInstrumentWidget(QString workspaceName);

  void handleActiveWorkspaceDeleted() override;

private:
  MantidWidgets::InstrumentWidget::TabCustomizations getTabCustomizations() const;

  std::vector<DetectorTube> findWholeTubeDetectorIndices(std::vector<std::size_t> const &partTubeDetectorIndices);
};

} // namespace MantidQt::CustomInterfaces
