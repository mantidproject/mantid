// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "SaveAlgorithmRunner.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"

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
                                               const bool &includeAdditionalColumns,
                                               std::string const &modelDescription, const bool &validateModel,
                                               const std::string &metaSource) const {
  auto alg = Mantid::API::AlgorithmManager::Instance().create("SaveISISReflectometryORSO");
  alg->setRethrows(true);
  alg->setProperty("WorkspaceList", workspaceNames);
  alg->setProperty("Filename", savePath);
  alg->setProperty("WriteResolution", includeQResolution);
  alg->setProperty("IncludeAdditionalColumns", includeAdditionalColumns);
  alg->setProperty("ModelDescription", modelDescription);
  alg->setProperty("ValidateModel", validateModel);
  alg->setProperty("MetadataSource", metaSource);
  if (metaSource == "History") {
    alg->execute();
    return;
  }
  API::InterfaceManager interfaceManager;
  QHash<QString, QString> presets;
  QStringList disabled;
  const std::vector<Mantid::Kernel::Property *> props = alg->getProperties();
  std::vector<Mantid::Kernel::Property *>::const_iterator p = props.cbegin();
  for (; p != props.cend(); ++p) {
    if (!(**p).isDefault()) {
      QString property_name = QString::fromStdString((**p).name());
      presets.insert(property_name, QString::fromStdString((**p).value()));
      disabled.append(property_name);
    }
  }
  auto dialog = dynamic_cast<MantidQt::API::AlgorithmDialog *>(
      interfaceManager.createDialog(alg, nullptr, false, presets, QString(), QStringList(), disabled));
  dialog->setModal(true);
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
