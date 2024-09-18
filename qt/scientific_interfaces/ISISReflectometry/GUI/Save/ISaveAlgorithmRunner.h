// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Workspace_fwd.h"
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class ISaveAlgorithmRunner {
public:
  virtual ~ISaveAlgorithmRunner() = default;

  virtual void runSaveAsciiAlgorithm(const Mantid::API::Workspace_sptr &workspace, std::string const &savePath,
                                     std::string const &extension, std::vector<std::string> const &logParameters,
                                     const bool &includeHeader, const bool &includeQResolution,
                                     std::string const &separator) const = 0;

  virtual void runSaveORSOAlgorithm(std::vector<std::string> const &workspaceNames, std::string const &savePath,
                                    const bool &includeQResolution, const bool &includeAdditionalColumns) const = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry