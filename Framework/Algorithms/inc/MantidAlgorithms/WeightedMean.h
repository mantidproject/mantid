// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/CommutativeBinaryOperation.h"
#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace Algorithms {
/** An algorithm to calculate the weighted mean of two workspaces.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the first input workspace.</LI>
    <LI> InputWorkspace2 - The name of the second input workspace. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result.</LI>
    </UL>

    @author Robert Dalgliesh, ISIS, RAL
    @date 12/1/2010
 */
class MANTID_ALGORITHMS_DLL WeightedMean : public CommutativeBinaryOperation {
public:
  const std::string name() const override { return "WeightedMean"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "An algorithm to calculate the weighted mean of two workspaces.";
  }

  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"Mean"}; }

private:
  // Overridden BinaryOperation methods
  void performBinaryOperation(const HistogramData::Histogram &lhs, const HistogramData::Histogram &rhs,
                              HistogramData::HistogramY &YOut, HistogramData::HistogramE &EOut) override;
  void performBinaryOperation(const HistogramData::Histogram &lhs, const double rhsY, const double rhsE,
                              HistogramData::HistogramY &YOut, HistogramData::HistogramE &EOut) override;
  bool checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                          const API::MatrixWorkspace_const_sptr rhs) const override;
  std::string checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                                     const API::MatrixWorkspace_const_sptr rhs) const override;

  /// The name of the first input workspace property for BinaryOperation
  std::string inputPropName1() const override { return "InputWorkspace1"; }
  /// The name of the second input workspace property for BinaryOperation
  std::string inputPropName2() const override { return "InputWorkspace2"; }
};

} // namespace Algorithms
} // namespace Mantid
