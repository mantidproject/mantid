// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include <boost/optional.hpp>
#include <map>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class Batch;
class Group;
class IConfiguredAlgorithm;

using AlgorithmRuntimeProps = std::map<std::string, std::string>;

MANTIDQT_ISISREFLECTOMETRY_DLL MantidQt::API::IConfiguredAlgorithm_sptr createConfiguredAlgorithm(Batch const &model,
                                                                                                  Group &group);
MANTIDQT_ISISREFLECTOMETRY_DLL AlgorithmRuntimeProps createAlgorithmRuntimeProps(Batch const &model,
                                                                                 Group const &group);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
