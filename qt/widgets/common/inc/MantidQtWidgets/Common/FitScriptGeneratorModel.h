// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <map>
#include <string>
#include <utility>

namespace MantidQt {
namespace MantidWidgets {

// Workspace name, workspace index
//using WorkspaceDomain = std::pair<std::string, std::size_t>;
// StartX, EndX
//using XRange = std::pair<double, double>;
// Fit functions
//using FitFunctions = std::vector<IFunction_sptr>;

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorModel {
public:
  FitScriptGeneratorModel();
  ~FitScriptGeneratorModel();

  void removeWorkspaceDomain(std::string const &workspaceName,
                             WorkspaceIndex workspaceIndex);
  void addWorkspaceDomain(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex, double startX,
                          double endX);

//private:
  //std::map<WorkspaceDomain, XRange> m_fitRanges;
  //std::map<WorkspaceDomain, FitFunctions> m_fitFunctions;
};

} // namespace MantidWidgets
} // namespace MantidQt
