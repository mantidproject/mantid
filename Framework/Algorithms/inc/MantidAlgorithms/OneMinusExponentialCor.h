// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_ONEMINUSEXPONENTIALCOR_H_
#define MANTID_ALGORITHMS_ONEMINUSEXPONENTIALCOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnaryOperation.h"

namespace Mantid {
namespace Algorithms {
/**
Corrects the data and error values on a workspace by one minus the value of an
exponential function
which is evaluated at the X value of each data point: c1(1-exp(-c*x)).
The data and error values are either divided or multiplied by the value of this
function.

Required Properties:
<UL>
<LI> InputWorkspace  - The name of the workspace to correct</LI>
<LI> OutputWorkspace - The name of the corrected workspace (can be the same as
the input one)</LI>
<LI> c               - The positive value by which the entire exponent
calculation is multiplied (see above)</LI>
<LI> c1              - The value by which the entire expression is multiplied
(see above)</LI>
<LI> Operation       - Whether to divide (the default) or multiply the data by
the correction function</LI>
</UL>

@author Russell Taylor, Tessella plc
@date 24/03/2009
*/
class DLLExport OneMinusExponentialCor : public UnaryOperation {
public:
  /// Default constructor
  OneMinusExponentialCor();
  /// Algorithm's name for identification
  const std::string name() const override { return "OneMinusExponentialCor"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Corrects the data in a workspace by one minus the value of an "
           "exponential function.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"MagFormFactorCorrection", "ExponentialCorrection",
            "PowerLawCorrection"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "CorrectionFunctions"; }

private:
  // Overridden UnaryOperation methods
  void defineProperties() override;
  void retrieveProperties() override;
  void performUnaryOperation(const double XIn, const double YIn,
                             const double EIn, double &YOut,
                             double &EOut) override;

  double m_c;    ///< The constant term in the exponent
  double m_c1;   ///< The multiplier
  bool m_divide; ///< Whether the data should be divided by the correction
                 ///(true) or multiplied by it (false)
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_ONEMINUSEXPONENTIALCOR_H_*/
