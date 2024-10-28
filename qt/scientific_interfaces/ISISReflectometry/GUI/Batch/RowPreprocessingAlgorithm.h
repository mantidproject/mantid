// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IBatch;
class PreviewRow;
class Item;
} // namespace MantidQt::CustomInterfaces::ISISReflectometry

namespace MantidQt::CustomInterfaces::ISISReflectometry::PreprocessRow {
MANTIDQT_ISISREFLECTOMETRY_DLL MantidQt::API::IConfiguredAlgorithm_sptr
createConfiguredAlgorithm(IBatch const &model, PreviewRow &row, Mantid::API::IAlgorithm_sptr alg = nullptr);
MANTIDQT_ISISREFLECTOMETRY_DLL void updateRowOnAlgorithmComplete(const Mantid::API::IAlgorithm_sptr &, Item &);
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::PreprocessRow
