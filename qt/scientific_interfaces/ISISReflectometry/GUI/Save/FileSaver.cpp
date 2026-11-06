// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FileSaver.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <algorithm>
#include <filesystem>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
static auto constexpr MULTI_DATASET_FILE_SUFFIX = "_multi";
} // unnamed namespace

namespace FileExtensions {
static auto constexpr CUSTOM = "custom";
static auto constexpr DAT = ".dat";
static auto constexpr TXT = ".txt";
static auto constexpr MFT = ".mft";
static auto constexpr ORT = ".ort";
static auto constexpr ORB = ".orb";
} // namespace FileExtensions

FileSaver::FileSaver(std::unique_ptr<ISaveAlgorithmRunner> saveAlgRunner, IFileHandler *fileHandler)
    : m_saveAlgRunner(std::move(saveAlgRunner)), m_fileHandler(fileHandler) {}

std::string FileSaver::extensionForFormat(NamedFormat format) {
  // For the custom format we need to pass just the word "custom" to the "extension" property of the save algorithm
  switch (format) {
  case NamedFormat::Custom:
    return FileExtensions::CUSTOM;
  case NamedFormat::ThreeColumn:
    return FileExtensions::DAT;
  case NamedFormat::ANSTO:
    return FileExtensions::TXT;
  case NamedFormat::ILLCosmos:
    return FileExtensions::MFT;
  case NamedFormat::ORSOAscii:
    return FileExtensions::ORT;
  case NamedFormat::ORSONexus:
    return FileExtensions::ORB;
  default:
    throw std::runtime_error("Unknown save format.");
  }
}

bool FileSaver::isValidSaveDirectory(std::string const &path) const { return m_fileHandler->fileExists(path); }

std::string FileSaver::assembleSavePath(std::string const &saveDirectory, std::string const &prefix,
                                        std::string const &name, std::string const &extension) const {
  std::filesystem::path path(saveDirectory);

  if (extension == FileExtensions::CUSTOM) {
    path /= (prefix + name + std::string(FileExtensions::DAT));
  } else if (extension == FileExtensions::ORT || extension == FileExtensions::ORB) {
    path /= (prefix + name + extension);
  } else {
    // The extension is added automatically for the rest of the formats
    path /= (prefix + name);
  }
  return path.string();
}

Mantid::API::Workspace_sptr FileSaver::workspace(std::string const &workspaceName) const {
  auto const &ads = Mantid::API::AnalysisDataService::Instance();

  if (!ads.doesExist(workspaceName)) {
    throw std::runtime_error("Cannot find workspace " + workspaceName + " in the ADS.");
  }

  return ads.retrieveWS<Mantid::API::Workspace>(workspaceName);
}

void FileSaver::runSaveAsciiAlgorithm(std::string const &savePath, std::string const &extension,
                                      Mantid::API::Workspace_sptr const &workspace,
                                      std::vector<std::string> const &logParameters,
                                      FileFormatOptions const &fileFormat) const {
  m_saveAlgRunner->runSaveAsciiAlgorithm(workspace, savePath, extension, logParameters,
                                         fileFormat.shouldIncludeHeader(), fileFormat.shouldIncludeQResolution(),
                                         fileFormat.separator());
}

void FileSaver::runSaveORSOAlgorithm(std::string const &savePath, std::vector<std::string> const &workspaceNames,
                                     FileFormatOptions const &fileFormat) const {
  m_saveAlgRunner->runSaveORSOAlgorithm(workspaceNames, savePath, fileFormat.shouldIncludeQResolution(),
                                        fileFormat.shouldIncludeAdditionalColumns(), fileFormat.model(),
                                        modelsForWorkspaces(workspaceNames, fileFormat), fileFormat.validate());
}

/** Get the model description for each workspace from the models keyed by run number.
 * @return : a list with one model for each workspace, or an empty list if the model
 * given for all workspaces should be used instead
 */
std::vector<std::string> FileSaver::modelsForWorkspaces(std::vector<std::string> const &workspaceNames,
                                                        FileFormatOptions const &fileFormat) const {
  auto const &runModels = fileFormat.runModels();
  if (!fileFormat.model().empty() || runModels.empty()) {
    return {};
  }

  std::vector<std::string> models;
  models.reserve(workspaceNames.size());
  for (auto const &name : workspaceNames) {
    auto const it = runModels.find(runNumber(name));
    models.emplace_back(it != runModels.cend() ? it->second : "");
  }

  if (std::all_of(models.cbegin(), models.cend(), [](auto const &model) { return model.empty(); })) {
    return {};
  }
  return models;
}

/** Get the run number from the logs of a workspace, or the first child of a workspace group.
 * @return : the run number, or an empty string if it cannot be found
 */
std::string FileSaver::runNumber(std::string const &workspaceName) const {
  auto ws = workspace(workspaceName);
  if (auto const group = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws)) {
    ws = group->size() > 0 ? group->getItem(0) : nullptr;
  }

  auto const matrixWs = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
  if (!matrixWs || !matrixWs->run().hasProperty("run_number")) {
    return "";
  }
  return matrixWs->run().getProperty("run_number")->value();
}

void FileSaver::save(Mantid::API::Workspace_sptr const &workspace, std::string const &saveDirectory,
                     std::vector<std::string> const &logParameters, FileFormatOptions const &fileFormat) const {
  auto const extension = extensionForFormat(fileFormat.format());
  auto const savePath = assembleSavePath(saveDirectory, fileFormat.prefix(), workspace->getName(), extension);

  if (fileFormat.isORSOFormat()) {
    std::vector<std::string> const workspaceNames{workspace->getName()};
    runSaveORSOAlgorithm(savePath, workspaceNames, fileFormat);
  } else {
    runSaveAsciiAlgorithm(savePath, extension, workspace, logParameters, fileFormat);
  }
}

void FileSaver::saveToSingleFile(std::vector<std::string> const &workspaceNames, std::string const &saveDirectory,
                                 FileFormatOptions const &fileFormat) const {
  auto const extension = extensionForFormat(fileFormat.format());
  auto const filename = workspaceNames.front() + MULTI_DATASET_FILE_SUFFIX;
  auto const savePath = assembleSavePath(saveDirectory, fileFormat.prefix(), filename, extension);

  if (fileFormat.isORSOFormat()) {
    runSaveORSOAlgorithm(savePath, workspaceNames, fileFormat);
  } else {
    throw std::invalid_argument(
        "Saving multiple workspaces to a single file is not supported for the selected file format.");
  }
}

void FileSaver::save(std::string const &saveDirectory, std::vector<std::string> const &workspaceNames,
                     std::vector<std::string> const &logParameters, FileFormatOptions const &fileFormat) const {
  if (!isValidSaveDirectory(saveDirectory)) {
    throw InvalidSavePath(saveDirectory);
  }

  // Setup the appropriate save algorithm
  if (shouldSaveToSingleFile(workspaceNames, fileFormat)) {
    saveToSingleFile(workspaceNames, saveDirectory, fileFormat);
  } else {
    for (auto const &name : workspaceNames) {
      auto const ws = workspace(name);
      if (ws->isGroup()) {
        // Save child workspaces into separate files
        Mantid::API::WorkspaceGroup_sptr group = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);
        for (auto child : group->getAllItems())
          save(child, saveDirectory, logParameters, fileFormat);
        continue;
      }

      save(ws, saveDirectory, logParameters, fileFormat);
    }
  }
}

bool FileSaver::shouldSaveToSingleFile(std::vector<std::string> const &workspaceNames,
                                       FileFormatOptions const &fileFormat) const {
  if (!fileFormat.shouldSaveToSingleFile() || !fileFormat.isORSOFormat()) {
    return false;
  }

  if (workspaceNames.size() > 1) {
    return true;
  }

  // If there is only one workspace name in the list then we may still have multiple datasets if it is a workspace group
  auto const ws = workspace(workspaceNames.front());
  if (!ws->isGroup()) {
    return false;
  }

  Mantid::API::WorkspaceGroup_sptr group = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);
  return group->size() > 1;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
