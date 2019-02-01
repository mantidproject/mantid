// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_PLUS_H_
#define MANTID_ALGORITHMS_PLUS_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/CommutativeBinaryOperation.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
/**
Plus performs the addition of two input workspaces.
It inherits from the Algorithm class, and overrides
the init() & exec() methods.

Required Properties:
<UL>
<LI> LHSWorkspace - The name of the workspace </LI>
<LI> RHSWorkspace - The name of the workspace </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the added
data </LI>
</UL>

@author Dickon Champion, RAL
@date 12/12/2007
*/
class DLLExport Plus : public CommutativeBinaryOperation {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Plus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The Plus algorithm will add the data values and calculate the "
           "corresponding error values in two compatible workspaces. ";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Divide", "Minus", "Multiply"};
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
  bool
  checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                     const API::MatrixWorkspace_const_sptr rhs) const override;
  void operateOnRun(const API::Run &lhs, const API::Run &rhs,
                    API::Run &ans) const override;

  // Overridden event-specific operation
  bool checkUnitCompatibility(const API::MatrixWorkspace_const_sptr lhs,
                              const API::MatrixWorkspace_const_sptr rhs) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_PLUS_H_*/
