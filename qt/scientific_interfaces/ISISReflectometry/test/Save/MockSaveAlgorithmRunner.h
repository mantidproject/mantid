// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Save/ISaveAlgorithmRunner.h"
#include <gmock/gmock.h>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MockSaveAlgorithmRunner : public ISaveAlgorithmRunner {
public:
  MOCK_CONST_METHOD7(runSaveAsciiAlgorithm,
                     void(const Mantid::API::Workspace_sptr &, std::string const &, std::string const &,
                          std::vector<std::string> const &, const bool &, const bool &, std::string const &));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry