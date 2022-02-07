// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY ONLY be included from a test in the MDAlgorithms package.
 *********************************************************************************/
#pragma once

#include "MantidDataObjects/MDHistoWorkspace.h"

namespace BinaryOperationMDTestHelper {
/// Run a binary algorithm.
DLLExport Mantid::DataObjects::MDHistoWorkspace_sptr doTest(const std::string &algoName, const std::string &lhs,
                                                            const std::string &rhs, const std::string &outName,
                                                            bool succeeds = true, const std::string &otherProp = "",
                                                            const std::string &otherPropValue = "");

} // namespace BinaryOperationMDTestHelper

namespace UnaryOperationMDTestHelper {
/// Run a unary algorithm.
DLLExport Mantid::DataObjects::MDHistoWorkspace_sptr doTest(const std::string &algoName, const std::string &inName,
                                                            const std::string &outName, bool succeeds = true,
                                                            const std::string &otherProp = "",
                                                            const std::string &otherPropValue = "");

} // namespace UnaryOperationMDTestHelper
