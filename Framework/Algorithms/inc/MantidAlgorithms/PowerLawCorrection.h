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
Corrects the data and error values on a workspace by the value of an exponential
function
which is evaluated at the X value of each data point: c0*x^C1.
The data and error values are multiplied by the value of this function.

Required Properties:
<UL>
<LI> InputWorkspace  - The name of the workspace to correct</LI>
<LI> OutputWorkspace - The name of the corrected workspace (can be the same as
the input one)</LI>
<LI> c0              - The value by which the entire calculation is multiplied
(see above)</LI>
<LI> c1              - The power by which the x value is raised (see above)</LI>
</UL>

@author Robert Dalgliesh ISIS, STFC
@date 14/10/2009
*/
class MANTID_ALGORITHMS_DLL PowerLawCorrection : public UnaryOperation {
public:
  /// Default constructor
  PowerLawCorrection();
  /// Algorithm's name for identification
  const std::string name() const override { return "PowerLawCorrection"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Corrects the data and error values on a workspace by the value of "
           "an exponential function which is evaluated at the X value of each "
           "data point: c0*x^C1. The data and error values are multiplied by "
           "the value of this function.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"PolynomialCorrection", "OneMinusExponentialCor", "MagFormFactorCorrection", "ExponentialCorrection"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "CorrectionFunctions"; }

private:
  // Overridden UnaryOperation methods
  void defineProperties() override;
  void retrieveProperties() override;
  void performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut, double &EOut) override;

  double m_c0; ///< The constant by which to multiply
  double m_c1; ///< The power to raise by.
};

} // namespace Algorithms
} // namespace Mantid
