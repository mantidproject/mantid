// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
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
Corrects the data and error values on a workspace by the value of a polynomial
function
which is evaluated at the X value of each data point. The data and error values
are multiplied or divided
by the value of this function.

Required Properties:
<UL>
<LI> InputWorkspace  - The name of the workspace to correct</LI>
<LI> OutputWorkspace - The name of the corrected workspace (can be the same as
the input one)</LI>
<LI> Coefficients    - The coefficients of the polynomial correction function in
ascending powers of X</LI>
<LI> Operatiopn      - Multiply/Divide The operation with which the correction
is applied to the data</LI>
</UL>

@author Russell Taylor, Tessella plc
@date 24/03/2009
*/
class MANTID_ALGORITHMS_DLL PolynomialCorrection : public UnaryOperation {
public:
  /// Default constructor
  PolynomialCorrection();
  /// Algorithm's name for identification
  const std::string name() const override { return "PolynomialCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Corrects the data in a workspace by the value of a polynomial "
           "function which is evaluated at the X value of each data point.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"OneMinusExponentialCor", "MagFormFactorCorrection", "ExponentialCorrection", "PowerLawCorrection"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "CorrectionFunctions"; }

private:
  // Overridden UnaryOperation methods
  void defineProperties() override;
  void retrieveProperties() override;
  void performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut, double &EOut) override;

  std::vector<double> m_coeffs; ///< Holds the coefficients for the polynomial
  /// correction function
  std::vector<double>::size_type m_polySize; ///< The order of the polynomial
  bool m_isOperationMultiply;                ///< True is the operation is multiply, false
  /// means divide
};

} // namespace Algorithms
} // namespace Mantid
