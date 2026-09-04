// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace.h"

#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

MANTIDQT_ISISREFLECTOMETRY_DLL std::vector<Mantid::API::MatrixWorkspace_sptr>
getMembers(Mantid::API::Workspace_sptr const &workspace);

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
