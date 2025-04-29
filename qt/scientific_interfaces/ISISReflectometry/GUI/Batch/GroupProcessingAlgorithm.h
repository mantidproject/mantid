// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "MantidAPI/IAlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class Batch;
class Group;
class IBatch;
class IConfiguredAlgorithm;
} // namespace MantidQt::CustomInterfaces::ISISReflectometry

namespace MantidQt::CustomInterfaces::ISISReflectometry::GroupProcessing {

using AlgorithmRuntimeProps = Mantid::API::IAlgorithmRuntimeProps;

MANTIDQT_ISISREFLECTOMETRY_DLL MantidQt::API::IConfiguredAlgorithm_sptr createConfiguredAlgorithm(IBatch const &model,
                                                                                                  Group &group);
MANTIDQT_ISISREFLECTOMETRY_DLL std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps>
createAlgorithmRuntimeProps(IBatch const &model, Group const &group);
} // namespace MantidQt::CustomInterfaces::ISISReflectometry::GroupProcessing
