#ifndef MANTID_API_WORKOPOVERLOADS_H_
#define MANTID_API_WORKOPOVERLOADS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace API
{

// Workspace operator overloads
Workspace_sptr DLLExport operator+(const Workspace_sptr lhs, const Workspace_sptr rhs);
Workspace_sptr DLLExport operator-(const Workspace_sptr lhs, const Workspace_sptr rhs);
Workspace_sptr DLLExport operator*(const Workspace_sptr lhs, const Workspace_sptr rhs);
Workspace_sptr DLLExport operator/(const Workspace_sptr lhs, const Workspace_sptr rhs);

Workspace_sptr DLLExport operator+(const Workspace_sptr lhs, const double& rhsValue);
Workspace_sptr DLLExport operator-(const Workspace_sptr lhs, const double& rhsValue);
Workspace_sptr DLLExport operator*(const Workspace_sptr lhs, const double& rhsValue);
Workspace_sptr DLLExport operator*(const double& lhsValue, const Workspace_sptr rhs);
Workspace_sptr DLLExport operator/(const Workspace_sptr lhs, const double& rhsValue);

Workspace_sptr DLLExport operator+=(const Workspace_sptr lhs, const Workspace_sptr rhs);
Workspace_sptr DLLExport operator-=(const Workspace_sptr lhs, const Workspace_sptr rhs);
Workspace_sptr DLLExport operator*=(const Workspace_sptr lhs, const Workspace_sptr rhs);
Workspace_sptr DLLExport operator/=(const Workspace_sptr lhs, const Workspace_sptr rhs);

Workspace_sptr DLLExport operator+=(const Workspace_sptr lhs, const double& rhsValue);
Workspace_sptr DLLExport operator-=(const Workspace_sptr lhs, const double& rhsValue);
Workspace_sptr DLLExport operator*=(const Workspace_sptr lhs, const double& rhsValue);
Workspace_sptr DLLExport operator/=(const Workspace_sptr lhs, const double& rhsValue);

/** A collection of static functions for use with workspaces

    @author Russell Taylor, Tessella Support Services plc
    @date 19/09/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
struct DLLExport WorkspaceHelpers
{
  // Checks whether a workspace has common X bins/values
  static const bool commonBoundaries(const Workspace_const_sptr WS);
  // Checks whether the binning is the same in two histograms
  static const bool matchingBins(const Workspace_const_sptr ws1,
                                 const Workspace_const_sptr ws2, const bool firstOnly = false);
  // Checks whether a the X vectors in a workspace are actually the same vector
  static const bool sharedXData(const Workspace_const_sptr WS);
  // Divides the data in a workspace by the bin width to make it a distribution (or the reverse)
  static void makeDistribution(Workspace_sptr workspace, const bool forwards = true);
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKOPOVERLOADS_H_ */
