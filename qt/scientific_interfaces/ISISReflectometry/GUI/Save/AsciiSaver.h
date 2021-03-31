// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "IAsciiSaver.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class AsciiSaver : public IAsciiSaver {
public:
  static Mantid::API::IAlgorithm_sptr getSaveAlgorithm();
  static std::string extensionForFormat(NamedFormat format);

  bool isValidSaveDirectory(std::string const &filePath) const override;
  void save(std::string const &saveDirectory, std::vector<std::string> const &workspaceNames,
            std::vector<std::string> const &logParameters, FileFormatOptions const &inputParameters) const override;

private:
  Mantid::API::IAlgorithm_sptr setUpSaveAlgorithm(std::string const &saveDirectory,
                                                  const Mantid::API::Workspace_sptr &workspace,
                                                  std::vector<std::string> const &logParameters,
                                                  FileFormatOptions const &fileFormat) const;

  std::string assembleSavePath(std::string const &saveDirectory, std::string const &prefix, std::string const &name,
                               std::string const &extension) const;

  Mantid::API::Workspace_sptr workspace(std::string const &workspaceName) const;
  void save(const Mantid::API::Workspace_sptr &workspace, std::string const &saveDirectory,
            std::vector<std::string> const &logParameters, FileFormatOptions const &fileFormat) const;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
