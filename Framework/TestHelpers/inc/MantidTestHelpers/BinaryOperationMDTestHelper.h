// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY ONLY be included from a test in the MDAlgorithms package.
 *********************************************************************************/
#ifndef MANTID_MDALGORITHMS_BINARYOPERATIONMDTESTHELPER_H_
#define MANTID_MDALGORITHMS_BINARYOPERATIONMDTESTHELPER_H_

#include "MantidDataObjects/MDHistoWorkspace.h"

namespace BinaryOperationMDTestHelper {
/// Run a binary algorithm.
DLLExport Mantid::DataObjects::MDHistoWorkspace_sptr
doTest(std::string algoName, std::string lhs, std::string rhs,
       std::string outName, bool succeeds = true, std::string otherProp = "",
       std::string otherPropValue = "");

} // namespace BinaryOperationMDTestHelper

namespace UnaryOperationMDTestHelper {
/// Run a unary algorithm.
DLLExport Mantid::DataObjects::MDHistoWorkspace_sptr
doTest(std::string algoName, std::string inName, std::string outName,
       bool succeeds = true, std::string otherProp = "",
       std::string otherPropValue = "");

} // namespace UnaryOperationMDTestHelper

#endif
