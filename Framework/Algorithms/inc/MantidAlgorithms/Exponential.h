// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnaryOperation.h"

namespace Mantid {
namespace Algorithms {
/**
Exponential performs the exponential function on an input workspace.
It inherits from the Algorithm class, and overrides
the init() & exec()  methods.

Required Properties:
<UL>
<LI> InputWorkspace1 - The name of the workspace </LI>
<LI> Exponent -        The value of the exponent to raise each value to </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the output
data </LI>
</UL>

@author Ron Fowler
@date 12/05/2010
*/
class MANTID_ALGORITHMS_DLL Exponential : public UnaryOperation {
public:
  /// Default constructor
  Exponential();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Exponential"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The Exponential algorithm will transform the signal values 'y' "
           "into e^y.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"Power", "Logarithm"}; }

private:
  // Overridden UnaryOperation methods
  void performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut, double &EOut) override;

  /*
  void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY,
  const MantidVec& lhsE,
                              const MantidVec& rhsY, const MantidVec& rhsE,
  MantidVec& YOut, MantidVec& EOut);
  void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY,
  const MantidVec& lhsE,
                              const double& rhsY, const double& rhsE, MantidVec&
  YOut, MantidVec& EOut);
  */
  // try and set output units field
  // void setOutputUnits(const API::MatrixWorkspace_const_sptr
  // lhs,API::MatrixWorkspace_sptr out);
};

} // namespace Algorithms
} // namespace Mantid
