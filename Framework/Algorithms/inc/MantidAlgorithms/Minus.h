// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MINUS_H_
#define MANTID_ALGORITHMS_MINUS_H_
#include "MantidAlgorithms/BinaryOperation.h"

namespace Mantid {
namespace Algorithms {
/**
Minus performs the difference of two input workspaces.
It inherits from the Algorithm class, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> LSHWorkspace - The name of the workspace </LI>
<LI> RHSWorkspace - The name of the workspace </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the
difference data </LI>
</UL>

@author Nick Draper
@date 14/12/2007
*/
class DLLExport Minus : public BinaryOperation {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Minus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The Minus algorithm will subtract the data values and calculate "
           "the corresponding error values for two compatible workspaces.";
  }

  /// Algorithm's alias for identification overriding a virtual method
  const std::string alias() const override;
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Plus", "Divide", "Multiply"};
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
  void performEventBinaryOperation(DataObjects::EventList &lhs,
                                   const DataObjects::EventList &rhs) override;
  void performEventBinaryOperation(DataObjects::EventList &lhs,
                                   const MantidVec &rhsX, const MantidVec &rhsY,
                                   const MantidVec &rhsE) override;
  void performEventBinaryOperation(DataObjects::EventList &lhs,
                                   const double &rhsY,
                                   const double &rhsE) override;

  void checkRequirements() override;
  std::string checkSizeCompatibility(
      const API::MatrixWorkspace_const_sptr lhs,
      const API::MatrixWorkspace_const_sptr rhs) const override;
  bool checkUnitCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                              const API::MatrixWorkspace_const_sptr rhs) const;
  bool
  checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                     const API::MatrixWorkspace_const_sptr rhs) const override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MINUS_H_*/
