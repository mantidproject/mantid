#ifndef MANTID_VATESAPI_BOXINFO_H
#define MANTID_VATESAPI_BOXINFO_H

#include "MantidKernel/System.h"
#include "MantidVatesAPI/WorkspaceProvider.h"
#include <boost/optional.hpp>

namespace Mantid {
namespace VATES {
/**
Collection of functions related to box information

@date 21/05/2015

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
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

#endif
