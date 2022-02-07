// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/BinaryOperation.h"

namespace Mantid {
namespace Algorithms {
/**
Divide performs the division of two input workspaces.
It inherits from the Algorithm class, and overrides
the init() & exec()  methods.

Required Properties:
<UL>
<LI> LHSWorkspace - The name of the workspace </LI>
<LI> RHSWorkspace - The name of the workspace </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the division
data </LI>
</UL>

@author Nick Draper
@date 14/12/2007
*/
class MANTID_ALGORITHMS_DLL Divide : public BinaryOperation {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Divide"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The Divide algorithm will divide the data values and calculate the "
           "corresponding error values of two compatible workspaces.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"Plus", "Minus", "Multiply"}; }

private:
  void init() override;
  void exec() override;
  // Overridden BinaryOperation methods
  void performBinaryOperation(const HistogramData::Histogram &lhs, const HistogramData::Histogram &rhs,
                              HistogramData::HistogramY &YOut, HistogramData::HistogramE &EOut) override;
  void performBinaryOperation(const HistogramData::Histogram &lhs, const double rhsY, const double rhsE,
                              HistogramData::HistogramY &YOut, HistogramData::HistogramE &EOut) override;
  void setOutputUnits(const API::MatrixWorkspace_const_sptr lhs, const API::MatrixWorkspace_const_sptr rhs,
                      API::MatrixWorkspace_sptr out) override;

  void performEventBinaryOperation(DataObjects::EventList &lhs, const DataObjects::EventList &rhs) override;

  void performEventBinaryOperation(DataObjects::EventList &lhs, const MantidVec &rhsX, const MantidVec &rhsY,
                                   const MantidVec &rhsE) override;

  void performEventBinaryOperation(DataObjects::EventList &lhs, const double &rhsY, const double &rhsE) override;

  void checkRequirements() override;

  std::string checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                                     const API::MatrixWorkspace_const_sptr rhs) const override;
  // usually you want to warn user if division by 0 occurs. set it to false to
  // generate these warnings on debug level only
  bool m_warnOnZeroDivide = true;
};

} // namespace Algorithms
} // namespace Mantid
