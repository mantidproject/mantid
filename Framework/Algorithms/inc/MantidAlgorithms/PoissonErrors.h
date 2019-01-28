// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_POISSONERRORS_H_
#define MANTID_ALGORITHMS_POISSONERRORS_H_

#include "MantidAlgorithms/BinaryOperation.h"

namespace Mantid {
namespace Algorithms {
/** Takes a Data workspace and an original counts workspace input and updates
   the
    error values in the data workspace to be the same fractionally as the counts
   workspace.
    The number of histograms, the binning and units of the two workspaces must
   match.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input</LI>
    <LI> CountsWorkspace - The name of the workspace that contains the original
   counts </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 03/02/2009
 */
class DLLExport PoissonErrors : public BinaryOperation {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PoissonErrors"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates the gaussian approxiamtion of Poisson error based on a "
           "matching workspace containing the original counts.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "SANS;Arithmetic\\Errors";
  }

private:
  // Overridden BinaryOperation methods
  void performBinaryOperation(const HistogramData::Histogram &lhs,
                              const HistogramData::Histogram &rhs,
                              HistogramData::HistogramY &YOut,
                              HistogramData::HistogramE &EOut) override;
  void performBinaryOperation(const HistogramData::Histogram &lhs,
                              const double rhsY, const double rhsE,
                              HistogramData::HistogramY &YOut,
                              HistogramData::HistogramE &EOut) override;
  std::string checkSizeCompatibility(
      const API::MatrixWorkspace_const_sptr lhs,
      const API::MatrixWorkspace_const_sptr rhs) const override;

  /// The name of the first input workspace property for BinaryOperation
  std::string inputPropName1() const override { return "InputWorkspace"; }
  /// The name of the second input workspace property for BinaryOperation
  std::string inputPropName2() const override { return "CountsWorkspace"; }
  /// The name of the output workspace property for BinaryOperation
  std::string outputPropName() const override { return "OutputWorkspace"; }
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_POISSONERRORS_H_*/
