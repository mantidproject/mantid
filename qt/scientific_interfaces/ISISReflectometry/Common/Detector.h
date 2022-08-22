// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

MANTIDQT_ISISREFLECTOMETRY_DLL bool hasLinearDetector(Mantid::API::MatrixWorkspace_sptr &ws);

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
