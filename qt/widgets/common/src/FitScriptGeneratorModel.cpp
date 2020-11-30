// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"

namespace MantidQt {
namespace MantidWidgets {

FitScriptGeneratorModel::FitScriptGeneratorModel() {}

FitScriptGeneratorModel::~FitScriptGeneratorModel() {}

void FitScriptGeneratorModel::removeWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) {}

void FitScriptGeneratorModel::addWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    double startX, double endX) {}

} // namespace MantidWidgets
} // namespace MantidQt
