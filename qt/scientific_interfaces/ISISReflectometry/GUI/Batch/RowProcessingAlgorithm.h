// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "MantidQtWidgets/Common/IAlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"
#include <boost/optional.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class IBatch;
class Row;

} // namespace MantidQt::CustomInterfaces::ISISReflectometry

namespace MantidQt::CustomInterfaces::ISISReflectometry::RowProcessing {

MANTIDQT_ISISREFLECTOMETRY_DLL MantidQt::API::IConfiguredAlgorithm_sptr createConfiguredAlgorithm(IBatch const &model,
                                                                                                  Row &row);
MANTIDQT_ISISREFLECTOMETRY_DLL std::unique_ptr<MantidQt::API::IAlgorithmRuntimeProps>
createAlgorithmRuntimeProps(IBatch const &model, Row const &row);
MANTIDQT_ISISREFLECTOMETRY_DLL std::unique_ptr<MantidQt::API::IAlgorithmRuntimeProps>
createAlgorithmRuntimeProps(IBatch const &model);
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::RowProcessing
