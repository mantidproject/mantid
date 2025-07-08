// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <functional>

namespace Mantid {
namespace API {

void renameWorkspacesInQENSFit(Algorithm *qensFit, IAlgorithm_sptr renameAlgorithm,
                               const WorkspaceGroup_sptr &outputGroup, std::string const &outputBaseName,
                               std::function<std::string(std::size_t)> const &getNameSuffix);

bool containsMultipleData(const std::vector<MatrixWorkspace_sptr> &workspaces);

} // namespace API
} // namespace Mantid
