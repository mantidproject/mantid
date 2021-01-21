// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IAsciiSaver.h"
namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
InvalidSavePath::InvalidSavePath(std::string const &path)
    : std::runtime_error("The path" + path + "does not exist or is not a directory."), m_path(path) {}

std::string const &InvalidSavePath::path() const { return m_path; }

InvalidWorkspaceName::InvalidWorkspaceName(std::string const &name)
    : std::runtime_error("Workspace " + name + " does not exist."), m_name(name) {}

std::string const &InvalidWorkspaceName::name() const { return m_name; }

FileFormatOptions::FileFormatOptions(NamedFormat format, std::string const &prefix, bool includeHeader,
                                     std::string const &separator, bool includeQResolution)
    : m_format(format), m_prefix(prefix), m_includeHeader(includeHeader), m_separator(separator),
      m_includeQResolution(includeQResolution) {}
bool FileFormatOptions::shouldIncludeHeader() const { return m_includeHeader; }
bool FileFormatOptions::shouldIncludeQResolution() const { return m_includeQResolution; }
std::string const &FileFormatOptions::separator() const { return m_separator; }
std::string const &FileFormatOptions::prefix() const { return m_prefix; }
NamedFormat FileFormatOptions::format() const { return m_format; }
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
