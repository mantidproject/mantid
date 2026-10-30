// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <optional>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

// Find the raw input, including periods moved into TOF groups after reduction.
// Requires an available reduction output with non-empty run_number metadata.
// Reduced workspaces provide run identity only; they are never returned as inputs.
MANTIDQT_ISISREFLECTOMETRY_DLL Mantid::API::MatrixWorkspace_sptr
findInputWorkspace(std::vector<std::string> const &inputs, std::optional<int> period = std::nullopt,
                   std::vector<std::string> const &reducedWorkspaces = {});

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
