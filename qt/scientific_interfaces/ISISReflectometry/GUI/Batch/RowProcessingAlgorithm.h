// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHM_H_
#define MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHM_H_

#include "Common/DllConfig.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include <boost/optional.hpp>
#include <map>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
class Batch;
class Row;
class IConfiguredAlgorithm;

using AlgorithmRuntimeProps = std::map<std::string, std::string>;

MANTIDQT_ISISREFLECTOMETRY_DLL MantidQt::API::IConfiguredAlgorithm_sptr
createConfiguredAlgorithm(Batch const &model, Row &row);

MANTIDQT_ISISREFLECTOMETRY_DLL AlgorithmRuntimeProps
createAlgorithmRuntimeProps(Batch const &model, Row const &row);
MANTIDQT_ISISREFLECTOMETRY_DLL AlgorithmRuntimeProps
createAlgorithmRuntimeProps(Batch const &model);
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_ROWPROCESSINGALGORITHM_H_
