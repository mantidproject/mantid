// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "SaveAlgorithmRunner.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

void SaveAlgorithmRunner::runSaveAsciiAlgorithm(const Mantid::API::Workspace_sptr &workspace,
                                                std::string const &savePath, std::string const &extension,
                                                std::vector<std::string> const &logParameters,
                                                const bool &includeHeader, const bool &includeQResolution,
                                                std::string const &separator) const {
  auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveReflectometryAscii");
  alg->setProperty("InputWorkspace", workspace);
  alg->setProperty("Filename", savePath);
  alg->setProperty("FileExtension", extension);
  alg->setProperty("LogList", logParameters);
  alg->setProperty("WriteHeader", includeHeader);
  alg->setProperty("WriteResolution", includeQResolution);
  alg->setProperty("Separator", separator);
  alg->execute();
}

void SaveAlgorithmRunner::runSaveORSOAlgorithm(std::vector<std::string> const &workspaceNames,
                                               std::string const &savePath, const bool &includeQResolution,
                                               const bool &includeAdditionalColumns) const {
  auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveISISReflectometryORSO");
  alg->setRethrows(true);
  alg->setProperty("WorkspaceList", workspaceNames);
  alg->setProperty("Filename", savePath);
  alg->setProperty("WriteResolution", includeQResolution);
  alg->setProperty("IncludeAdditionalColumns", includeAdditionalColumns);
  alg->execute();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
