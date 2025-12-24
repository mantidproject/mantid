// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"
#include "Reduction/Item.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IBatch;
class PreviewRow;
class Row;
} // namespace MantidQt::CustomInterfaces::ISISReflectometry

namespace MantidQt::CustomInterfaces::ISISReflectometry::Reduction {
// These functions concern reduction of a workspace using ReflectometryReductionOneAuto. This is used to perform
// just the reduction step on its own when performing on-the-fly reduction on the Preview tab
MANTIDQT_ISISREFLECTOMETRY_DLL MantidQt::API::IConfiguredAlgorithm_sptr
createConfiguredAlgorithm(IBatch const &model, PreviewRow &row, Mantid::API::IAlgorithm_sptr alg = nullptr);
MANTIDQT_ISISREFLECTOMETRY_DLL std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps>
createAlgorithmRuntimeProps(IBatch const &model, PreviewRow const &row);
MANTIDQT_ISISREFLECTOMETRY_DLL void updateRowOnAlgorithmComplete(const Mantid::API::IAlgorithm_sptr &algorithm,
                                                                 Item &item);
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::Reduction

namespace MantidQt::CustomInterfaces::ISISReflectometry::RowProcessing {
// These functions concern the full loading, preprocessing, and reduction of a set of input runs using
// ReflectometryISISLoadAndProcess. This is used to perform full processing for each row on the Runs table.
// This is included in the same file as the reduction algorithm because it shares mostly the same properties.
MANTIDQT_ISISREFLECTOMETRY_DLL MantidQt::API::IConfiguredAlgorithm_sptr createConfiguredAlgorithm(IBatch const &model,
                                                                                                  Row &row);
MANTIDQT_ISISREFLECTOMETRY_DLL std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps>
createAlgorithmRuntimeProps(IBatch const &model, std::optional<std::reference_wrapper<Row const>> row = std::nullopt);
MANTIDQT_ISISREFLECTOMETRY_DLL std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps>
createAlgorithmRuntimePropsDefault(IBatch const &model);
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::RowProcessing
