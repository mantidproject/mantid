#ifndef MANTID_ISISREFLECTOMETRY_REFLASCIISAVER_H
#define MANTID_ISISREFLECTOMETRY_REFLASCIISAVER_H
#include <vector>
#include <string>
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "IReflAsciiSaver.h"

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
}
}
#endif // MANTID_ISISREFLECTOMETRY_REFLASCIISAVER_H
