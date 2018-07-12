#include "ReflAsciiSaver.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <Poco/Path.h>
#include <Poco/File.h>
namespace MantidQt {
namespace CustomInterfaces {

Mantid::API::IAlgorithm_sptr
ReflAsciiSaver::algorithmForFormat(NamedFormat format) {
  auto create =
      [](std::string const &algorithmName) -> Mantid::API::IAlgorithm_sptr {
        return Mantid::API::AlgorithmManager::Instance().create(algorithmName);
      };
  switch (format) {
  case NamedFormat::MFT:
    return create("SaveMFT");
  case NamedFormat::ANSTO:
    return create("SaveANSTOAscii");
  default:
    throw std::runtime_error("Unknown save format.");
  }
}

std::string ReflAsciiSaver::extensionForFormat(NamedFormat format) {
  switch (format) {
  case NamedFormat::MFT:
    return ".mft";
  case NamedFormat::ANSTO:
    return ".txt";
  default:
    throw std::runtime_error("Unknown save format.");
  }
}

bool ReflAsciiSaver::isValidSaveDirectory(std::string const &path) const {
  if (!path.empty()) {
    try {
      auto pocoPath = Poco::Path().parseDirectory(path);
      auto pocoFile = Poco::File(pocoPath);
      return pocoFile.exists() && pocoFile.canWrite();
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
}

std::string ReflAsciiSaver::assembleSavePath(
    std::string const &saveDirectory, std::string const &prefix,
    std::string const &name, std::string const &extension) const {
  auto path = Poco::Path(saveDirectory).makeDirectory();
  path.append(prefix + name + extension);
  return path.toString();
}

Mantid::API::MatrixWorkspace_sptr
ReflAsciiSaver::workspace(std::string const &workspaceName) const {
  auto const &ads = Mantid::API::AnalysisDataService::Instance();

  if (!ads.doesExist(workspaceName))
    return nullptr;

  return ads.retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
}

Mantid::API::IAlgorithm_sptr ReflAsciiSaver::setUpSaveAlgorithm(
    std::string const &saveDirectory,
    Mantid::API::MatrixWorkspace_sptr workspace,
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

void ReflAsciiSaver::save(std::string const &saveDirectory,
                          std::vector<std::string> const &workspaceNames,
                          std::vector<std::string> const &logParameters,
                          FileFormatOptions const &fileFormat) const {
  // Setup the appropriate save algorithm
  if (isValidSaveDirectory(saveDirectory)) {
    for (auto const &name : workspaceNames) {
      auto ws = workspace(name);
      if (!ws)
        throw InvalidWorkspaceName(name);

      auto alg =
          setUpSaveAlgorithm(saveDirectory, ws, logParameters, fileFormat);
      alg->execute();
    }
  } else {
    throw InvalidSavePath(saveDirectory);
  }
}
}
}
