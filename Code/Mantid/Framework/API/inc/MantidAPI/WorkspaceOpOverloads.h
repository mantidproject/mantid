#ifndef MANTID_API_WORKOPOVERLOADS_H_
#define MANTID_API_WORKOPOVERLOADS_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

namespace OperatorOverloads {
// Helper function for operator overloads
template <typename LHSType, typename RHSType, typename ResultType>
DLLExport ResultType
    executeBinaryOperation(const std::string &algorithmName, const LHSType lhs,
                           const RHSType rhs, bool lhsAsOutput = false,
                           bool child = true, const std::string &name = "",
                           bool rethrow = false);
}

bool MANTID_API_DLL equals(const MatrixWorkspace_sptr lhs,
                           const MatrixWorkspace_sptr rhs,
                           double tolerance = 0.0);

// Workspace operator overloads
MatrixWorkspace_sptr MANTID_API_DLL
operator+(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr MANTID_API_DLL
operator-(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr MANTID_API_DLL
operator*(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr MANTID_API_DLL
operator/(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);

MatrixWorkspace_sptr MANTID_API_DLL
operator+(const MatrixWorkspace_sptr lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL
operator-(const MatrixWorkspace_sptr lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL
operator-(const double &lhsValue, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr MANTID_API_DLL
operator*(const MatrixWorkspace_sptr lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL
operator*(const double &lhsValue, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr MANTID_API_DLL
operator/(const MatrixWorkspace_sptr lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL
operator/(const double &lhsValue, const MatrixWorkspace_sptr rhs);

MatrixWorkspace_sptr MANTID_API_DLL
operator+=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr MANTID_API_DLL
operator-=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr MANTID_API_DLL
operator*=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);
MatrixWorkspace_sptr MANTID_API_DLL
operator/=(const MatrixWorkspace_sptr lhs, const MatrixWorkspace_sptr rhs);

MatrixWorkspace_sptr MANTID_API_DLL
operator+=(const MatrixWorkspace_sptr lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL
operator-=(const MatrixWorkspace_sptr lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL
operator*=(const MatrixWorkspace_sptr lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL
operator/=(const MatrixWorkspace_sptr lhs, const double &rhsValue);

/** A collection of static functions for use with workspaces

    @author Russell Taylor, Tessella Support Services plc
    @date 19/09/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
struct MANTID_API_DLL WorkspaceHelpers {
  // Checks whether a workspace has common X bins/values
  static bool commonBoundaries(const MatrixWorkspace_const_sptr WS);
  // Checks whether the binning is the same in two histograms
  static bool matchingBins(const MatrixWorkspace_const_sptr ws1,
                           const MatrixWorkspace_const_sptr ws2,
                           const bool firstOnly = false);
  // Checks whether a the X vectors in a workspace are actually the same vector
  static bool sharedXData(const MatrixWorkspace_const_sptr WS);
  // Divides the data in a workspace by the bin width to make it a distribution
  // (or the reverse)
  static void makeDistribution(MatrixWorkspace_sptr workspace,
                               const bool forwards = true);
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKOPOVERLOADS_H_ */
