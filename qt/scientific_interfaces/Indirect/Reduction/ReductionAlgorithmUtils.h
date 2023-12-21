// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllConfig.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"

#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces {

MANTIDQT_INDIRECT_DLL MantidQt::API::IConfiguredAlgorithm_sptr loadConfiguredAlg(std::string const &filename,
                                                                                 std::string const &instrument,
                                                                                 std::vector<int> const &detectorList,
                                                                                 std::string const &outputWorkspace);

MANTIDQT_INDIRECT_DLL MantidQt::API::IConfiguredAlgorithm_sptr
calculateFlatBackgroundConfiguredAlg(std::string const &inputWorkspace, double const startX, double const endX,
                                     std::string const &outputWorkspace);

MANTIDQT_INDIRECT_DLL MantidQt::API::IConfiguredAlgorithm_sptr
groupDetectorsConfiguredAlg(std::string const &inputWorkspace, std::vector<int> const &detectorList,
                            std::string const &outputWorkspace);

} // namespace MantidQt::CustomInterfaces