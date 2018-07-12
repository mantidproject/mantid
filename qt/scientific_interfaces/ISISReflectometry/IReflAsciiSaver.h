#ifndef MANTID_ISISREFLECTOMETRY_IREFLASCIISAVER_H
#define MANTID_ISISREFLECTOMETRY_IREFLASCIISAVER_H
#include <vector>
#include <string>
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
namespace MantidQt {
namespace CustomInterfaces {

enum class NamedFormat { MFT, ANSTO };

class FileFormatOptions {
public:
  FileFormatOptions(NamedFormat format, std::string const &prefix,
                    bool includeTitle, std::string const &separator,
                    bool includeQResolution);
  bool shouldIncludeTitle() const;
  bool shouldIncludeQResolution() const;
  std::string const &separator() const;
  std::string const &prefix() const;
  NamedFormat format() const;

private:
  NamedFormat m_format;
  std::string m_prefix;
  bool m_includeTitle;
  std::string m_separator;
  bool m_includeQResolution;
};

class InvalidSavePath : public std::runtime_error {
public:
  explicit InvalidSavePath(std::string const &path);
  std::string const &path() const;

private:
  std::string m_path;
};

class InvalidWorkspaceName : public std::runtime_error {
public:
  explicit InvalidWorkspaceName(std::string const &path);
  std::string const &name() const;

private:
  std::string m_name;
};

class IReflAsciiSaver {
public:
  virtual bool isValidSaveDirectory(std::string const &filePath) const = 0;
  virtual void save(std::string const &saveDirectory,
                    std::vector<std::string> const &workspaceNames,
                    std::vector<std::string> const &logParameters,
                    FileFormatOptions const &inputParameters) const = 0;
  virtual ~IReflAsciiSaver() = default;
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_IREFLASCIISAVER_H
