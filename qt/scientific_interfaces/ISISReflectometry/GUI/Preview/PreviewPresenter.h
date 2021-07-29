// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IPreviewView.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class PreviewPresenter : public PreviewViewSubscriber {
  void notifyLoadWorkspaceRequested() override;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
