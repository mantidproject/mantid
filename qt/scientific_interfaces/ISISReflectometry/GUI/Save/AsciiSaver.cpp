// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "AsciiSaver.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include <Poco/File.h>
#include <Poco/Path.h>

#include <utility>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

Mantid::API::IAlgorithm_sptr AsciiSaver::getSaveAlgorithm() {
  return Mantid::API::AlgorithmManager::Instance().create("SaveReflectometryAscii");
}

std::string AsciiSaver::extensionForFormat(NamedFormat format) {
  // The algorithm is slightly inconsistent in that for the custom format the
  // "extension" property is not really an extension but just the word "custom"
  switch (format) {
  case NamedFormat::Custom:
    return "custom";
  case NamedFormat::ThreeColumn:
    return ".dat";
  case NamedFormat::ANSTO:
    return ".txt";
  case NamedFormat::ILLCosmos:
    return ".mft";
  default:
    throw std::runtime_error("Unknown save format.");
  }
}

bool AsciiSaver::isValidSaveDirectory(std::string const &path) const {
  if (!path.empty()) {
    try {
      auto pocoPath = Poco::Path().parseDirectory(path);
      auto pocoFile = Poco::File(pocoPath);
      return pocoFile.exists();
    } catch (Poco::PathSyntaxException &) {
      return false;
    }
  } else
    return false;
}

std::string AsciiSaver::assembleSavePath(std::string const &saveDirectory, std::string const &prefix,
                                         std::string const &name, std::string const &extension) const {
  auto path = Poco::Path(saveDirectory).makeDirectory();
  // The extension is added automatically except where it is "custom"
  if (extension == "custom")
    path.append(prefix + name + std::string(".dat"));
  else
    path.append(prefix + name);

  return path.toString();
}

Mantid::API::Workspace_sptr AsciiSaver::workspace(std::string const &workspaceName) const {
  auto const &ads = Mantid::API::AnalysisDataService::Instance();

  if (!ads.doesExist(workspaceName))
    return nullptr;

  return ads.retrieveWS<Mantid::API::Workspace>(workspaceName);
}

Mantid::API::IAlgorithm_sptr AsciiSaver::setUpSaveAlgorithm(std::string const &saveDirectory,
                                                            const Mantid::API::Workspace_sptr &workspace,
                                                            std::vector<std::string> const &logParameters,
                                                            FileFormatOptions const &fileFormat) const {
  auto const saveAlg = getSaveAlgorithm();
  auto const extension = extensionForFormat(fileFormat.format());
  auto const savePath = assembleSavePath(saveDirectory, fileFormat.prefix(), workspace->getName(), extension);

  saveAlg->setProperty("InputWorkspace", workspace);
  saveAlg->setProperty("Filename", savePath);
  saveAlg->setProperty("FileExtension", extension);
  saveAlg->setProperty("LogList", logParameters);
  saveAlg->setProperty("WriteHeader", fileFormat.shouldIncludeHeader());
  saveAlg->setProperty("WriteResolution", fileFormat.shouldIncludeQResolution());
  saveAlg->setProperty("Separator", fileFormat.separator());
  return saveAlg;
}

void AsciiSaver::save(const Mantid::API::Workspace_sptr &workspace, std::string const &saveDirectory,
                      std::vector<std::string> const &logParameters, FileFormatOptions const &fileFormat) const {
  auto alg = setUpSaveAlgorithm(saveDirectory, std::move(workspace), logParameters, fileFormat);
  alg->execute();
}

void AsciiSaver::save(std::string const &saveDirectory, std::vector<std::string> const &workspaceNames,
                      std::vector<std::string> const &logParameters, FileFormatOptions const &fileFormat) const {
  // Setup the appropriate save algorithm
  if (isValidSaveDirectory(saveDirectory)) {
    for (auto const &name : workspaceNames) {
      auto ws = workspace(name);
      if (ws->isGroup()) {
        // Save child workspaces separately because the current algorithms
        // don't handle groups. When we switch to SaveReflectometryAscii we can
        // probably remove this
        Mantid::API::WorkspaceGroup_sptr group = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);
        for (auto child : group->getAllItems())
          save(child, saveDirectory, logParameters, fileFormat);
        continue;
      }

      save(ws, saveDirectory, logParameters, fileFormat);
    }
  } else {
    throw InvalidSavePath(saveDirectory);
  }
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
