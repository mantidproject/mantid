// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IFileSaver.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

InvalidSavePath::InvalidSavePath(std::string const &path)
    : std::runtime_error("The path" + path + "does not exist or is not a directory."), m_path(path) {}

InvalidSavePath::~InvalidSavePath() = default;

std::string const &InvalidSavePath::path() const { return m_path; }

InvalidWorkspaceName::InvalidWorkspaceName(std::string const &name)
    : std::runtime_error("Workspace " + name + " does not exist."), m_name(name) {}

InvalidWorkspaceName::~InvalidWorkspaceName() = default;

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

ORSOModelOptions::ORSOModelOptions(std::map<std::string, std::string> workspaceModels, bool validate)
    : m_workspaceModels(std::move(workspaceModels)), m_validate(validate) {}
std::map<std::string, std::string> const &ORSOModelOptions::workspaceModels() const { return m_workspaceModels; }
bool ORSOModelOptions::validate() const { return m_validate; }

/** Get the model description for a workspace
 * @return : the model, or an empty string if the workspace has no model
 */
std::string ORSOModelOptions::model(std::string const &workspaceName) const {
  auto const it = m_workspaceModels.find(workspaceName);
  return it != m_workspaceModels.cend() ? it->second : "";
}

/** Get the model descriptions for a list of workspaces
 * @return : a list with one model for each workspace, with an empty string for a workspace that has no model
 */
std::vector<std::string> ORSOModelOptions::models(std::vector<std::string> const &workspaceNames) const {
  std::vector<std::string> models;
  models.reserve(workspaceNames.size());
  std::transform(workspaceNames.cbegin(), workspaceNames.cend(), std::back_inserter(models),
                 [this](auto const &workspaceName) { return model(workspaceName); });
  return models;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
