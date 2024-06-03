// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <stdexcept>
#include <string>
#include <vector>
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

enum class NamedFormat { Custom, ThreeColumn, ANSTO, ILLCosmos, ORSOAscii, ORSONexus };

class MANTIDQT_ISISREFLECTOMETRY_DLL FileFormatOptions {
public:
  FileFormatOptions(NamedFormat format, std::string prefix, bool includeHeader, std::string separator,
                    bool includeQResolution, bool includeAdditionalColumns, bool saveToSingleFile);
  bool shouldIncludeHeader() const;
  bool shouldIncludeQResolution() const;
  bool shouldIncludeAdditionalColumns() const;
  bool shouldSaveToSingleFile() const;
  std::string const &separator() const;
  std::string const &prefix() const;
  NamedFormat format() const;
  bool isORSOFormat() const;

private:
  NamedFormat m_format;
  std::string m_prefix;
  bool m_includeHeader;
  std::string m_separator;
  bool m_includeQResolution;
  bool m_includeAdditionalCols;
  bool m_saveToSingleFile;
};

class InvalidSavePath : public std::runtime_error {
public:
  explicit InvalidSavePath(std::string const &path);
  std::string const &path() const;

private:
  std::string m_path;
};

inline bool operator==(const FileFormatOptions &lhs, const FileFormatOptions &rhs) {
  return lhs.format() == rhs.format() && lhs.shouldIncludeHeader() == rhs.shouldIncludeHeader() &&
         lhs.shouldIncludeQResolution() == rhs.shouldIncludeQResolution() && lhs.separator() == rhs.separator() &&
         lhs.prefix() == rhs.prefix();
}

inline bool operator!=(const FileFormatOptions &lhs, const FileFormatOptions &rhs) { return !(lhs == rhs); }

class InvalidWorkspaceName : public std::runtime_error {
public:
  explicit InvalidWorkspaceName(std::string const &path);
  std::string const &name() const;

private:
  std::string m_name;
};

class IFileSaver {
public:
  virtual bool isValidSaveDirectory(std::string const &filePath) const = 0;
  virtual void save(std::string const &saveDirectory, std::vector<std::string> const &workspaceNames,
                    std::vector<std::string> const &logParameters, FileFormatOptions const &inputParameters) const = 0;
  virtual ~IFileSaver() = default;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
