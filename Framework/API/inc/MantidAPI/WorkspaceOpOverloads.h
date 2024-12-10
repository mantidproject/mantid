// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <string>

namespace Mantid {
namespace API {

namespace OperatorOverloads {
// Helper function for operator overloads
template <typename LHSType, typename RHSType, typename ResultType>
DLLExport ResultType executeBinaryOperation(const std::string &algorithmName, const LHSType lhs, const RHSType rhs,
                                            bool lhsAsOutput = false, bool child = true, const std::string &name = "",
                                            bool rethrow = false);
} // namespace OperatorOverloads

bool MANTID_API_DLL equals(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs, double tolerance = 0.0);

// Workspace operator overloads
MatrixWorkspace_sptr MANTID_API_DLL operator+(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs);
MatrixWorkspace_sptr MANTID_API_DLL operator-(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs);
MatrixWorkspace_sptr MANTID_API_DLL operator*(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs);
MatrixWorkspace_sptr MANTID_API_DLL operator/(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs);

MatrixWorkspace_sptr MANTID_API_DLL operator+(const MatrixWorkspace_sptr &lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL operator-(const MatrixWorkspace_sptr &lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL operator-(const double &lhsValue, const MatrixWorkspace_sptr &rhs);
MatrixWorkspace_sptr MANTID_API_DLL operator*(const MatrixWorkspace_sptr &lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL operator*(const double &lhsValue, const MatrixWorkspace_sptr &rhs);
MatrixWorkspace_sptr MANTID_API_DLL operator/(const MatrixWorkspace_sptr &lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL operator/(const double &lhsValue, const MatrixWorkspace_sptr &rhs);

MatrixWorkspace_sptr MANTID_API_DLL operator+=(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs);
MatrixWorkspace_sptr MANTID_API_DLL operator-=(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs);
MatrixWorkspace_sptr MANTID_API_DLL operator*=(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs);
MatrixWorkspace_sptr MANTID_API_DLL operator/=(const MatrixWorkspace_sptr &lhs, const MatrixWorkspace_sptr &rhs);

MatrixWorkspace_sptr MANTID_API_DLL operator+=(const MatrixWorkspace_sptr &lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL operator-=(const MatrixWorkspace_sptr &lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL operator*=(const MatrixWorkspace_sptr &lhs, const double &rhsValue);
MatrixWorkspace_sptr MANTID_API_DLL operator/=(const MatrixWorkspace_sptr &lhs, const double &rhsValue);

/** A collection of static functions for use with workspaces

    @author Russell Taylor, Tessella Support Services plc
    @date 19/09/2008
*/
struct MANTID_API_DLL WorkspaceHelpers {
  // Checks whether the binning is the same in two workspaces
  static bool matchingBins(const std::shared_ptr<const MatrixWorkspace> &ws1,
                           const std::shared_ptr<const MatrixWorkspace> &ws2, const bool firstOnly = false);
  // Checks whether a the X vectors in a workspace are actually the same vector
  static bool sharedXData(const std::shared_ptr<const MatrixWorkspace> &WS);
  // Divides the data in a workspace by the bin width to make it a distribution
  // (or the reverse)
  static void makeDistribution(const MatrixWorkspace_sptr &workspace, const bool forwards = true);
};

} // namespace API
} // namespace Mantid
