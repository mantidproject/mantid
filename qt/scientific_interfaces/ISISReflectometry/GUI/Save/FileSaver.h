// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "GUI/Common/IFileHandler.h"
#include "IFileSaver.h"
#include "ISaveAlgorithmRunner.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class MANTIDQT_ISISREFLECTOMETRY_DLL FileSaver : public IFileSaver {

public:
  FileSaver(std::unique_ptr<ISaveAlgorithmRunner> saveAlgRunner, IFileHandler *fileHandler);
  static std::string extensionForFormat(NamedFormat format);

  bool isValidSaveDirectory(std::string const &filePath) const override;
  void save(std::string const &saveDirectory, std::vector<std::string> const &workspaceNames,
            std::vector<std::string> const &logParameters, FileFormatOptions const &inputParameters) const override;

private:
  std::unique_ptr<ISaveAlgorithmRunner> m_saveAlgRunner;
  IFileHandler *m_fileHandler;

  void runSaveAsciiAlgorithm(std::string const &savePath, std::string const &extension,
                             const Mantid::API::Workspace_sptr &workspace,
                             std::vector<std::string> const &logParameters, FileFormatOptions const &fileFormat) const;

  void runSaveORSOAlgorithm(std::string const &savePath, std::vector<std::string> const &workspaceNames,
                            FileFormatOptions const &fileFormat) const;

  std::string assembleSavePath(std::string const &saveDirectory, std::string const &prefix, std::string const &name,
                               std::string const &extension) const;

  Mantid::API::Workspace_sptr workspace(std::string const &workspaceName) const;
  void save(const Mantid::API::Workspace_sptr &workspace, std::string const &saveDirectory,
            std::vector<std::string> const &logParameters, FileFormatOptions const &fileFormat) const;

  void saveToSingleFile(std::vector<std::string> const &workspaceNames, std::string const &saveDirectory,
                        FileFormatOptions const &fileFormat) const;

  bool shouldSaveToSingleFile(std::vector<std::string> const &workspaceNames,
                              FileFormatOptions const &fileFormat) const;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
