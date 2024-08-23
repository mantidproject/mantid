// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FileSaver.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include <Poco/Path.h>

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
static constexpr auto MULTI_DATASET_FILE_SUFFIX = "_multi";
} // unnamed namespace

namespace FileExtensions {
static constexpr auto CUSTOM = "custom";
static constexpr auto DAT = ".dat";
static constexpr auto TXT = ".txt";
static constexpr auto MFT = ".mft";
static constexpr auto ORT = ".ort";
static constexpr auto ORB = ".orb";
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
  auto path = Poco::Path(saveDirectory).makeDirectory();

  if (extension == FileExtensions::CUSTOM) {
    path.append(prefix + name + std::string(FileExtensions::DAT));
  } else if (extension == FileExtensions::ORT || extension == FileExtensions::ORB) {
    path.append(prefix + name + extension);
  } else {
    // The extension is added automatically for the rest of the formats
    path.append(prefix + name);
  }
  return path.toString();
}

Mantid::API::Workspace_sptr FileSaver::workspace(std::string const &workspaceName) const {
  auto const &ads = Mantid::API::AnalysisDataService::Instance();

  if (!ads.doesExist(workspaceName)) {
    throw std::runtime_error("Cannot find workspace " + workspaceName + " in the ADS.");
  }

  return ads.retrieveWS<Mantid::API::Workspace>(workspaceName);
}

void FileSaver::runSaveAsciiAlgorithm(std::string const &savePath, std::string const &extension,
                                      const Mantid::API::Workspace_sptr &workspace,
                                      std::vector<std::string> const &logParameters,
                                      FileFormatOptions const &fileFormat) const {
  m_saveAlgRunner->runSaveAsciiAlgorithm(workspace, savePath, extension, logParameters,
                                         fileFormat.shouldIncludeHeader(), fileFormat.shouldIncludeQResolution(),
                                         fileFormat.separator());
}

void FileSaver::runSaveORSOAlgorithm(std::string const &savePath, std::vector<std::string> const &workspaceNames,
                                     FileFormatOptions const &fileFormat) const {
  m_saveAlgRunner->runSaveORSOAlgorithm(workspaceNames, savePath, fileFormat.shouldIncludeQResolution(),
                                        fileFormat.shouldIncludeAdditionalColumns());
}

void FileSaver::save(const Mantid::API::Workspace_sptr &workspace, std::string const &saveDirectory,
                     std::vector<std::string> const &logParameters, FileFormatOptions const &fileFormat) const {
  auto const extension = extensionForFormat(fileFormat.format());
  auto const savePath = assembleSavePath(saveDirectory, fileFormat.prefix(), workspace->getName(), extension);

  if (fileFormat.isORSOFormat()) {
    const std::vector<std::string> workspaceNames{workspace->getName()};
    runSaveORSOAlgorithm(savePath, workspaceNames, fileFormat);
  } else {
    runSaveAsciiAlgorithm(savePath, extension, workspace, logParameters, fileFormat);
  }
}

void FileSaver::saveToSingleFile(std::vector<std::string> const &workspaceNames, std::string const &saveDirectory,
                                 FileFormatOptions const &fileFormat) const {
  auto const extension = extensionForFormat(fileFormat.format());
  auto filename = workspaceNames.front() + MULTI_DATASET_FILE_SUFFIX;
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
      auto ws = workspace(name);
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
  const auto ws = workspace(workspaceNames.front());
  if (!ws->isGroup()) {
    return false;
  }

  Mantid::API::WorkspaceGroup_sptr group = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);
  return group->size() > 1;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
