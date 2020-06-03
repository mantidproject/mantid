// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include <boost/optional.hpp>

namespace Mantid {
namespace VATES {
/**
Collection of functions related to box information

@date 21/05/2015
*/

/**
 * Function for finding an appropriate initial recursion depth.
 * @param workspaceName :: name of the workspace
 * @param workspaceProvider :: an instance of the a workspace provider
 * @returns the appropriate recursion depth or nothing
 */
boost::optional<int> DLLExport findRecursionDepthForTopLevelSplitting(
    const std::string &workspaceName,
    const WorkspaceProvider &workspaceProvider);
} // namespace VATES
} // namespace Mantid
