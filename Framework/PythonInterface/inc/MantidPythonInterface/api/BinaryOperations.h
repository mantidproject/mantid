// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_BINARYOPERATIONS_H_
#define MANTID_PYTHONINTERFACE_BINARYOPERATIONS_H_
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>

namespace Mantid {
namespace PythonInterface {
/**
  Defines helpers to run the binary operation algorithms
*/
/** @name Binary operation helpers */
//@{
/// Binary op for two workspaces
template <typename LHSType, typename RHSType, typename ResultType>
ResultType performBinaryOp(const LHSType lhs, const RHSType rhs,
                           const std::string &op, const std::string &name,
                           bool inplace, bool reverse);

/// Binary op for two MDworkspaces
template <typename LHSType, typename RHSType, typename ResultType>
ResultType performBinaryOpMD(const LHSType lhs, const RHSType rhs,
                             const std::string &op, const std::string &name,
                             bool inplace, bool reverse);

/// Binary op for a workspace and a double
template <typename LHSType, typename ResultType>
ResultType performBinaryOpWithDouble(const LHSType inputWS, const double value,
                                     const std::string &op,
                                     const std::string &name, bool inplace,
                                     bool reverse);

/// Binary op for MDworkspaces + double
template <typename LHSType, typename ResultType>
ResultType performBinaryOpMDWithDouble(const LHSType lhs, const double value,
                                       const std::string &op,
                                       const std::string &name, bool inplace,
                                       bool reverse);
//@}
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_BINARYOPERATIONS_H_ */
