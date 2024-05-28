// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IFileSaver.h"

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

InvalidSavePath::InvalidSavePath(std::string const &path)
    : std::runtime_error("The path" + path + "does not exist or is not a directory."), m_path(path) {}

std::string const &InvalidSavePath::path() const { return m_path; }

InvalidWorkspaceName::InvalidWorkspaceName(std::string const &name)
    : std::runtime_error("Workspace " + name + " does not exist."), m_name(name) {}

std::string const &InvalidWorkspaceName::name() const { return m_name; }

FileFormatOptions::FileFormatOptions(NamedFormat format, std::string prefix, bool includeHeader, std::string separator,
                                     bool includeQResolution, bool includeAdditionalColumns, bool saveToSingleFile)
    : m_format(format), m_prefix(std::move(prefix)), m_includeHeader(includeHeader), m_separator(std::move(separator)),
      m_includeQResolution(includeQResolution), m_includeAdditionalCols(includeAdditionalColumns),
      m_saveToSingleFile(saveToSingleFile) {}
bool FileFormatOptions::shouldIncludeHeader() const { return m_includeHeader; }
bool FileFormatOptions::shouldIncludeQResolution() const { return m_includeQResolution; }
bool FileFormatOptions::shouldIncludeAdditionalColumns() const { return m_includeAdditionalCols; }
bool FileFormatOptions::shouldSaveToSingleFile() const { return m_saveToSingleFile; }
std::string const &FileFormatOptions::separator() const { return m_separator; }
std::string const &FileFormatOptions::prefix() const { return m_prefix; }
NamedFormat FileFormatOptions::format() const { return m_format; }
bool FileFormatOptions::isORSOFormat() const {
  switch (m_format) {
  case NamedFormat::ORSOAscii:
    return true;
  case NamedFormat::ORSONexus:
    return true;
  default:
    return false;
  }
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
