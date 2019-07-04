// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IASCIISAVER_H
#define MANTID_ISISREFLECTOMETRY_IASCIISAVER_H
#include "Common/DllConfig.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <string>
#include <vector>
namespace MantidQt {
namespace CustomInterfaces {

enum class NamedFormat { Custom, ThreeColumn, ANSTO, ILLCosmos };

class MANTIDQT_ISISREFLECTOMETRY_DLL FileFormatOptions {
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

inline bool operator==(const FileFormatOptions &lhs,
                       const FileFormatOptions &rhs) {
  return lhs.format() == rhs.format() &&
         lhs.shouldIncludeTitle() == rhs.shouldIncludeTitle() &&
         lhs.shouldIncludeQResolution() == rhs.shouldIncludeQResolution() &&
         lhs.separator() == rhs.separator() && lhs.prefix() == rhs.prefix();
}

inline bool operator!=(const FileFormatOptions &lhs,
                       const FileFormatOptions &rhs) {
  return !(lhs == rhs);
}

class InvalidWorkspaceName : public std::runtime_error {
public:
  explicit InvalidWorkspaceName(std::string const &path);
  std::string const &name() const;

private:
  std::string m_name;
};

class IAsciiSaver {
public:
  virtual bool isValidSaveDirectory(std::string const &filePath) const = 0;
  virtual void save(std::string const &saveDirectory,
                    std::vector<std::string> const &workspaceNames,
                    std::vector<std::string> const &logParameters,
                    FileFormatOptions const &inputParameters) const = 0;
  virtual ~IAsciiSaver() = default;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_IASCIISAVER_H
