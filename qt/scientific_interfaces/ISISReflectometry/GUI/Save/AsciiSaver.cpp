// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "AsciiSaver.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include <Poco/File.h>
#include <Poco/Path.h>
namespace MantidQt {
namespace CustomInterfaces {

Mantid::API::IAlgorithm_sptr
AsciiSaver::algorithmForFormat(NamedFormat format) {
  auto create =
      [](std::string const &algorithmName) -> Mantid::API::IAlgorithm_sptr {
    return Mantid::API::AlgorithmManager::Instance().create(algorithmName);
  };
  switch (format) {
  case NamedFormat::Custom:
    return create("SaveReflCustomAscii");
  case NamedFormat::ThreeColumn:
    return create("SaveReflThreeColumnAscii");
  case NamedFormat::ANSTO:
    return create("SaveANSTOAscii");
  case NamedFormat::ILLCosmos:
    return create("SaveILLCosmosAscii");
  default:
    throw std::runtime_error("Unknown save format.");
  }
}

std::string AsciiSaver::extensionForFormat(NamedFormat format) {
  switch (format) {
  case NamedFormat::Custom:
    return ".dat";
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

namespace {
template <typename T>
void setPropertyIfSupported(Mantid::API::IAlgorithm_sptr alg,
                            std::string const &propertyName, T const &value) {
  if (alg->existsProperty(propertyName))
    alg->setProperty(propertyName, value);
}
} // namespace

std::string AsciiSaver::assembleSavePath(std::string const &saveDirectory,
                                         std::string const &prefix,
                                         std::string const &name,
                                         std::string const &extension) const {
  auto path = Poco::Path(saveDirectory).makeDirectory();
  path.append(prefix + name + extension);
  return path.toString();
}

Mantid::API::Workspace_sptr
AsciiSaver::workspace(std::string const &workspaceName) const {
  auto const &ads = Mantid::API::AnalysisDataService::Instance();

  if (!ads.doesExist(workspaceName))
    return nullptr;

  return ads.retrieveWS<Mantid::API::Workspace>(workspaceName);
}

Mantid::API::IAlgorithm_sptr
AsciiSaver::setUpSaveAlgorithm(std::string const &saveDirectory,
                               Mantid::API::Workspace_sptr workspace,
                               std::vector<std::string> const &logParameters,
                               FileFormatOptions const &fileFormat) const {
  auto saveAlg = algorithmForFormat(fileFormat.format());
  auto extension = extensionForFormat(fileFormat.format());
  if (fileFormat.shouldIncludeTitle())
    setPropertyIfSupported(saveAlg, "Title", workspace->getTitle());

  setPropertyIfSupported(saveAlg, "LogList", logParameters);
  setPropertyIfSupported(saveAlg, "WriteDeltaQ",
                         fileFormat.shouldIncludeQResolution());
  saveAlg->setProperty("Separator", fileFormat.separator());
  saveAlg->setProperty("Filename",
                       assembleSavePath(saveDirectory, fileFormat.prefix(),
                                        workspace->getName(), extension));
  saveAlg->setProperty("InputWorkspace", workspace);
  return saveAlg;
}

void AsciiSaver::save(Mantid::API::Workspace_sptr workspace,
                      std::string const &saveDirectory,
                      std::vector<std::string> const &logParameters,
                      FileFormatOptions const &fileFormat) const {
  auto alg =
      setUpSaveAlgorithm(saveDirectory, workspace, logParameters, fileFormat);
  alg->execute();
}

void AsciiSaver::save(std::string const &saveDirectory,
                      std::vector<std::string> const &workspaceNames,
                      std::vector<std::string> const &logParameters,
                      FileFormatOptions const &fileFormat) const {
  // Setup the appropriate save algorithm
  if (isValidSaveDirectory(saveDirectory)) {
    for (auto const &name : workspaceNames) {
      auto ws = workspace(name);
      if (ws->isGroup()) {
        // Save child workspaces separately because the current algorithms
        // don't handle groups. When we switch to SaveReflectometryAscii we can
        // probably remove this
        Mantid::API::WorkspaceGroup_sptr group =
            boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);
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
} // namespace CustomInterfaces
} // namespace MantidQt
