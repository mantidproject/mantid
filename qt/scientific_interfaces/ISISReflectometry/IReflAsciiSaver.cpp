#include "IReflAsciiSaver.h"
namespace MantidQt {
namespace CustomInterfaces {
InvalidSavePath::InvalidSavePath(std::string const &path)
    : std::runtime_error("The path" + path +
                         "does not exist or is not a directory."),
      m_path(path) {}
std::string const &InvalidSavePath::path() const { return m_path; }

FileFormatOptions::FileFormatOptions(NamedFormat format,
                                     std::string const &prefix,
                                     bool includeTitle,
                                     std::string const &separator,
                                     bool includeQResolution)
    : m_format(format), m_prefix(prefix), m_includeTitle(includeTitle),
      m_separator(separator), m_includeQResolution(includeQResolution) {}
bool FileFormatOptions::shouldIncludeTitle() const { return m_includeTitle; }
bool FileFormatOptions::shouldIncludeQResolution() const {
  return m_includeQResolution;
}
std::string const &FileFormatOptions::separator() const { return m_separator; }
std::string const &FileFormatOptions::prefix() const { return m_prefix; }
NamedFormat FileFormatOptions::format() const { return m_format; }
}
}
