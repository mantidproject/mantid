// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLASCIISAVER_H
#define MANTID_ISISREFLECTOMETRY_REFLASCIISAVER_H
#include "IReflAsciiSaver.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
class ReflAsciiSaver : public IReflAsciiSaver {
public:
  static Mantid::API::IAlgorithm_sptr algorithmForFormat(NamedFormat format);
  static std::string extensionForFormat(NamedFormat format);

  bool isValidSaveDirectory(std::string const &filePath) const override;
  void save(std::string const &saveDirectory,
            std::vector<std::string> const &workspaceNames,
            std::vector<std::string> const &logParameters,
            FileFormatOptions const &inputParameters) const override;

private:
  Mantid::API::IAlgorithm_sptr
  setUpSaveAlgorithm(std::string const &saveDirectory,
                     Mantid::API::MatrixWorkspace_sptr workspace,
                     std::vector<std::string> const &logParameters,
                     FileFormatOptions const &fileFormat) const;

  std::string assembleSavePath(std::string const &saveDirectory,
                               std::string const &prefix,
                               std::string const &name,
                               std::string const &extension) const;

  Mantid::API::MatrixWorkspace_sptr
  workspace(std::string const &workspaceName) const;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_REFLASCIISAVER_H
