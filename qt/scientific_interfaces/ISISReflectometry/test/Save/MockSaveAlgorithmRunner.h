// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Save/ISaveAlgorithmRunner.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MockSaveAlgorithmRunner : public ISaveAlgorithmRunner {
public:
  MOCK_CONST_METHOD7(runSaveAsciiAlgorithm,
                     void(const Mantid::API::Workspace_sptr &, std::string const &, std::string const &,
                          std::vector<std::string> const &, const bool &, const bool &, std::string const &));
  MOCK_CONST_METHOD4(runSaveORSOAlgorithm,
                     void(std::vector<std::string> const &, std::string const &, const bool &, const bool &));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
GNU_DIAG_ON_SUGGEST_OVERRIDE
